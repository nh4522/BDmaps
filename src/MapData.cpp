#include "MapData.h"
#include <mapbox/earcut.hpp>
#include <nlohmann/json.hpp>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <filesystem>

using json = nlohmann::json;

static std::vector<uint32_t> earcutPoly(const std::vector<glm::vec2>& poly) {
    std::vector<std::vector<std::array<float, 2>>> rings(1);
    for (auto& p : poly) rings[0].push_back({ p.x, p.y });
    return mapbox::earcut<uint32_t>(rings);
}

static int getDivisionIndex(const std::string& divisionName) {
    for (int i = 0; i < (int)DIVISIONS.size(); ++i) {
        if (DIVISIONS[i].name == divisionName)
            return i;
    }
    // Try to match partial names
    std::string divLower = divisionName;
    std::transform(divLower.begin(), divLower.end(), divLower.begin(), ::tolower);
    for (int i = 0; i < (int)DIVISIONS.size(); ++i) {
        std::string divNameLower = DIVISIONS[i].name;
        std::transform(divNameLower.begin(), divNameLower.end(), divNameLower.begin(), ::tolower);
        if (divLower.find(divNameLower) != std::string::npos) {
            return i;
        }
    }
    std::cout << "[MapData] Warning: Unknown division: " << divisionName << std::endl;
    return 0;
}

MapData::MapData(const std::string& path) {
    std::cout << "[MapData] Constructor called with path: " << path << std::endl;

    // Check if file exists
    std::ifstream f(path);
    if (f.good()) {
        std::cout << "[MapData] GeoJSON file found, loading..." << std::endl;
        loadGeoJSON(path);
    }
    else {
        std::cout << "[MapData] GeoJSON file not found at: " << path << std::endl;
        std::cout << "[MapData] Current working directory: " << std::filesystem::current_path() << std::endl;
        std::cout << "[MapData] Using built-in data.\n";
        buildEmbeddedData();
    }

    if (m_districts.empty()) {
        std::cout << "[MapData] ERROR: No districts loaded! Using fallback data." << std::endl;
        buildEmbeddedData();
    }

    projectCoordinates();
    computeCentroids();
    buildGPUBuffers();

    for (int i = 0; i < (int)m_districts.size(); ++i)
        m_districts[i].phaseOffset = (float)i * 0.47f;

    std::cout << "[MapData] Initialization complete. Loaded " << m_districts.size() << " districts." << std::endl;
}

MapData::~MapData() {
    for (auto& d : m_districts) {
        if (d.vao) { glDeleteVertexArrays(1, &d.vao);     glDeleteBuffers(1, &d.vbo); }
        if (d.sideVao) { glDeleteVertexArrays(1, &d.sideVao); glDeleteBuffers(1, &d.sideVbo); }
        if (d.wireVao) { glDeleteVertexArrays(1, &d.wireVao); glDeleteBuffers(1, &d.wireVbo); }
    }
}

void MapData::loadGeoJSON(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[MapData] Error: Could not open " << path << std::endl;
        return;
    }

    try {
        json root = json::parse(f);
        std::cout << "[MapData] Successfully parsed JSON" << std::endl;

        if (!root.contains("features")) {
            std::cerr << "[MapData] Error: No 'features' array in GeoJSON" << std::endl;
            return;
        }

        int featureCount = 0;
        for (auto& feature : root["features"]) {
            District d;
            auto& props = feature["properties"];

            // Extract district information from GADM properties
            if (props.contains("NAME_2")) {
                d.name = props["NAME_2"];
            }
            else if (props.contains("name")) {
                d.name = props["name"];
            }
            else {
                d.name = "Unknown";
            }

            if (props.contains("NAME_1")) {
                d.division = props["NAME_1"];
            }
            else if (props.contains("division")) {
                d.division = props["division"];
            }
            else {
                d.division = "Unknown";
            }

            d.area = props.value("Shape_Area", props.value("area", 0.f));
            d.population = props.value("population", 0);
            d.capital = props.value("capital", "");
            d.divisionIdx = getDivisionIndex(d.division);

            auto& geom = feature["geometry"];
            if (!geom.contains("coordinates")) {
                std::cerr << "[MapData] Warning: Feature has no coordinates" << std::endl;
                continue;
            }

            std::string gt = geom["type"];

            // Handle both Polygon and MultiPolygon
            try {
                if (gt == "Polygon") {
                    // Single polygon
                    auto coords = geom["coordinates"][0];
                    for (auto& pt : coords) {
                        d.polygon.push_back({ pt[0].get<double>(), pt[1].get<double>() });
                    }
                }
                else if (gt == "MultiPolygon") {
                    // MultiPolygon - take the first polygon (largest one)
                    // Or you could merge multiple polygons, but for districts we take the main one
                    auto& polygons = geom["coordinates"];
                    size_t largestPolygonIdx = 0;
                    size_t largestSize = 0;

                    // Find the largest polygon (likely the main district area)
                    for (size_t i = 0; i < polygons.size(); ++i) {
                        auto& coords = polygons[i][0];
                        if (coords.size() > largestSize) {
                            largestSize = coords.size();
                            largestPolygonIdx = i;
                        }
                    }

                    // Use the largest polygon
                    auto& coords = polygons[largestPolygonIdx][0];
                    for (auto& pt : coords) {
                        d.polygon.push_back({ pt[0].get<double>(), pt[1].get<double>() });
                    }

                    // Log if there were multiple polygons
                    if (polygons.size() > 1) {
                        std::cout << "[MapData] District " << d.name << " has " << polygons.size()
                            << " polygons, using largest with " << d.polygon.size() << " points" << std::endl;
                    }
                }
                else {
                    std::cerr << "[MapData] Warning: Unsupported geometry type: " << gt << std::endl;
                    continue;
                }

                if (d.polygon.size() >= 3) {
                    m_districts.push_back(std::move(d));
                    featureCount++;
                }
                else {
                    std::cerr << "[MapData] Warning: Invalid polygon for " << d.name
                        << " (only " << d.polygon.size() << " points)" << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[MapData] Error parsing coordinates for " << d.name << ": " << e.what() << std::endl;
            }
        }

        std::cout << "[MapData] Loaded " << featureCount << " districts from GeoJSON." << std::endl;

    }
    catch (const json::parse_error& e) {
        std::cerr << "[MapData] JSON parse error: " << e.what() << std::endl;
        std::cerr << "[MapData] Byte position of error: " << e.byte << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[MapData] Error loading GeoJSON: " << e.what() << std::endl;
    }
}

void MapData::buildEmbeddedData() {
    std::cout << "[MapData] Building embedded district data..." << std::endl;

    // Simplified embedded data as fallback
    struct DistrictData {
        std::string name;
        std::string division;
        std::vector<std::pair<double, double>> points;
    };

    std::vector<DistrictData> districts = {
        // Barisal Division
        {"Barguna", "Barisal", {{89.9848,21.9577}, {89.9862,21.9571}, {89.9836,21.954}, {89.9833,21.9564}, {89.9848,21.9577}}},
        {"Barisal", "Barisal", {{90.4317,22.4906}, {90.4397,22.4825}, {90.4417,22.4739}, {90.4397,22.4639}, {90.4331,22.4589}, {90.4292,22.46}, {90.4261,22.4636}, {90.4231,22.4742}, {90.4189,22.4786}, {90.385,22.4881}, {90.3839,22.4992}, {90.38,22.5067}, {90.3911,22.5039}, {90.4042,22.4914}, {90.4072,22.495}, {90.41,22.5047}, {90.4117,22.5047}, {90.4139,22.5014}, {90.4169,22.5014}, {90.4242,22.5092}, {90.4325,22.5119}, {90.4339,22.5083}, {90.4292,22.4981}, {90.4317,22.4906}}},
        {"Bhola", "Barisal", {{90.8153,21.8857}, {90.8161,21.8883}, {90.8264,21.8924}, {90.827,21.8893}, {90.8222,21.8849}, {90.8184,21.8841}, {90.8153,21.8857}}},
        {"Jhalokati", "Barisal", {{90.1008,22.3583}, {90.0996,22.3586}, {90.095,22.3542}, {90.0892,22.3453}, {90.0864,22.346}, {90.0793,22.3526}, {90.07,22.3422}, {90.0616,22.3436}, {90.0509,22.3369}}},
        {"Patuakhali", "Barisal", {{90.4566,21.8076}, {90.4578,21.8076}, {90.4576,21.8041}, {90.457,21.8053}, {90.4566,21.8076}}},
        {"Pirojpur", "Barisal", {{89.9301,22.1622}, {89.9064,22.1585}, {89.9008,22.1636}, {89.8915,22.1978}, {89.8781,22.2268}, {89.8769,22.2382}, {89.8711,22.2529}, {89.8694,22.2635}, {89.8711,22.2764}, {89.8686,22.2806}, {89.8698,22.2823}, {89.872,22.2823}, {89.8717,22.2883}, {89.8831,22.3127}, {89.9007,22.3392}, {89.9061,22.3553}, {89.9081,22.3737}, {89.9105,22.3769}, {89.9137,22.3756}, {89.9125,22.3797}, {89.9221,22.3954}, {89.9424,22.4108}, {89.9633,22.4228}, {89.977,22.4389}, {89.9877,22.4655}, {89.9907,22.4817}, {89.9886,22.4825}, {89.9878,22.4864}, {89.9894,22.4925}, {89.9921,22.494}, {90.0017,22.4914}, {90.0133,22.4908}, {90.018,22.4945}, {90.015,22.5006}, {90.0021,22.5014}, {89.9958,22.5034}, {89.9844,22.5003}, {89.981,22.498}, {89.9786,22.4936}, {89.9769,22.4937}, {89.976,22.4903}, {89.9781,22.4864}, {89.9794,22.4747}, {89.9733,22.4533}, {89.952,22.44}, {89.9206,22.4086}, {89.9042,22.395}, {89.894,22.39}, {89.8933,22.3881}, {89.8751,22.4062}, {89.8715,22.4116}, {89.8709,22.4179}, {89.8761,22.4276}, {89.8863,22.4305}, {89.8932,22.4388}, {89.9016,22.4441}, {89.9109,22.4473}, {89.9253,22.4454}, {89.9331,22.4479}, {89.9426,22.4568}, {89.9445,22.4656}, {89.941,22.4735}, {89.9307,22.4779}, {89.924,22.4777}, {89.9092,22.4734}, {89.8937,22.4764}, {89.8883,22.4807}, {89.8834,22.4873}, {89.8819,22.4963}, {89.8846,22.5027}, {89.8916,22.5068}, {89.9138,22.5054}, {89.9317,22.4968}, {89.9441,22.4932}, {89.9584,22.4918}, {89.965,22.4927}, {89.9696,22.5014}, {89.9681,22.5058}, {89.9646,22.5077}, {89.9318,22.5081}, {89.9234,22.5103}, {89.9294,22.5226}, {89.9444,22.5381}, {89.9542,22.554}, {89.9581,22.5643}, {89.9591,22.5749}, {89.9612,22.5786}, {89.9623,22.5933}, {89.9607,22.5966}, {89.9569,22.5989}, {89.92,22.6067}, {89.9239,22.621}, {89.927,22.6418}, {89.9224,22.6464}, {89.8964,22.656}, {89.8901,22.6632}, {89.8928,22.6726}, {89.8967,22.675}, {89.9007,22.6721}, {89.9092,22.6595}, {89.9139,22.6597}, {89.9247,22.6695}, {89.9356,22.6768}, {89.9369,22.6802}, {89.9305,22.6857}, {89.9166,22.6929}, {89.9021,22.6954}, {89.8682,22.6916}, {89.8636,22.6942}, {89.8614,22.699}, {89.8632,22.7029}, {89.8686,22.7068}, {89.8747,22.71}, {89.876,22.709}, {89.8781,22.7099}, {89.8807,22.7129}, {89.9046,22.7258}, {89.9123,22.7264}, {89.9282,22.722}, {89.932,22.7242}, {89.9332,22.7393}, {89.9314,22.7524}, {89.9275,22.7543}, {89.9155,22.7515}, {89.9134,22.7524}, {89.9102,22.7578}, {89.9094,22.7644}, {89.9044,22.7723}, {89.9054,22.7744}, {89.9088,22.7758}, {89.9169,22.7772}, {89.9174,22.7805}, {89.9216,22.7841}, {89.9329,22.782}, {89.9383,22.7849}, {89.9328,22.7929}, {89.9278,22.7966}, {89.9234,22.7959}, {89.914,22.7998}, {89.9155,22.8075}, {89.924,22.8122}, {89.939,22.8245}, {89.9448,22.8264}, {89.9458,22.8317}, {89.9403,22.8454}, {89.9434,22.8478}, {89.9438,22.8566}, {89.9385,22.8625}, {89.9385,22.8647}, {89.9456,22.8626}, {89.9512,22.8565}, {89.9534,22.8561}, {89.9539,22.8539}, {89.9621,22.8529}, {89.967,22.8537}, {89.9706,22.859}, {89.9771,22.8588}, {89.9863,22.8618}, {89.9873,22.8658}, {90.0127,22.8674}, {90.0184,22.871}, {90.0203,22.8681}, {90.0348,22.8657}, {90.0344,22.8387}, {90.0277,22.8175}, {90.0372,22.8182}, {90.0439,22.8244}, {90.0544,22.8269}, {90.0516,22.8179}, {90.0545,22.8129}, {90.0511,22.8102}, {90.0606,22.8071}, {90.0682,22.8008}, {90.0767,22.7848}, {90.0872,22.7837}, {90.0942,22.7853}, {90.1006,22.7821}, {90.0983,22.7773}, {90.099,22.7755}, {90.1054,22.7744}, {90.1079,22.7757}, {90.1085,22.7783}, {90.1147,22.7781}, {90.1218,22.7747}, {90.1227,22.7706}, {90.1274,22.7697}, {90.1295,22.7716}, {90.1345,22.7669}, {90.1369,22.7688}, {90.1414,22.7686}, {90.1407,22.7598}, {90.1445,22.7571}, {90.1461,22.7582}, {90.1512,22.7574}, {90.1517,22.7629}, {90.1615,22.7592}, {90.1668,22.7605}, {90.1648,22.752}, {90.1669,22.7479}, {90.1712,22.7474}, {90.1737,22.7526}, {90.1776,22.7527}, {90.1833,22.7458}, {90.1888,22.7445}, {90.1863,22.7434}, {90.1877,22.742}, {90.1865,22.7407}, {90.1818,22.7393}, {90.1795,22.7351}, {90.1727,22.7312}, {90.1725,22.7294}, {90.1656,22.7279}, {90.1647,22.7235}, {90.162,22.7207}, {90.1581,22.7196}, {90.1567,22.7152}, {90.1579,22.7124}, {90.1553,22.7048}, {90.1516,22.704}, {90.1493,22.6935}, {90.143,22.6867}, {90.1422,22.6806}, {90.1389,22.6775}, {90.1382,22.6736}, {90.117,22.6662}, {90.1077,22.659}, {90.1011,22.658}, {90.0972,22.6556}, {90.0993,22.652}, {90.0965,22.6462}, {90.0965,22.6396}, {90.1013,22.6375}, {90.1004,22.6263}, {90.1064,22.6211}, {90.1101,22.6098}, {90.1022,22.5961}, {90.0918,22.5905}, {90.0788,22.5878}, {90.0743,22.5884}, {90.0643,22.5949}, {90.059,22.5829}, {90.0559,22.5798}, {90.0598,22.5732}, {90.0605,22.5607}, {90.0657,22.5528}, {90.0643,22.5345}, {90.0674,22.5292}, {90.0649,22.5283}, {90.0669,22.5182}, {90.0629,22.5051}, {90.0728,22.5116}, {90.075,22.5074}, {90.0704,22.5006}, {90.0715,22.4988}, {90.0743,22.4987}, {90.079,22.4956}, {90.0774,22.4923}, {90.0791,22.4876}, {90.0853,22.4879}, {90.089,22.4922}, {90.0889,22.4954}, {90.0988,22.4968}, {90.1009,22.4996}, {90.1096,22.4939}, {90.1136,22.4936}, {90.1159,22.4954}, {90.1136,22.4995}, {90.1169,22.503}, {90.1234,22.4981}, {90.1226,22.4959}, {90.1249,22.4899}, {90.1269,22.4914}, {90.1318,22.4912}, {90.134,22.4869}, {90.1332,22.4835}, {90.1293,22.478}, {90.1258,22.477}, {90.1244,22.4697}, {90.1218,22.4666}, {90.1104,22.4587}, {90.1057,22.4619}, {90.1054,22.4539}, {90.1007,22.4447}, {90.0922,22.4448}, {90.0903,22.4442}, {90.0902,22.442}, {90.0832,22.442}, {90.0814,22.4393}, {90.0772,22.4399}, {90.0742,22.4521}, {90.0697,22.4524}, {90.0692,22.454}, {90.068,22.4532}, {90.0677,22.4561}, {90.0652,22.4576}, {90.0656,22.462}, {90.0636,22.4653}, {90.0608,22.4614}, {90.0579,22.4612}, {90.0564,22.456}, {90.0565,22.4534}, {90.0607,22.451}, {90.0617,22.4454}, {90.06,22.4432}, {90.0555,22.4417}, {90.0426,22.4438}, {90.0363,22.4486}, {90.0273,22.4442}, {90.0305,22.4371}, {90.0303,22.4332}, {90.0275,22.4284}, {90.0285,22.4202}, {90.0321,22.4183}, {90.0344,22.408}, {90.0388,22.4023}, {90.0312,22.3963}, {90.0254,22.3961}, {90.0228,22.3945}, {90.0236,22.3869}, {90.019,22.3718}, {90.0235,22.3681}, {90.031,22.3673}, {90.0337,22.3631}, {90.0354,22.365}, {90.036,22.3751}, {90.0402,22.3681}, {90.0448,22.3644}, {90.0464,22.3603}, {90.0517,22.3598}, {90.0559,22.3568}, {90.0494,22.3353}, {90.0536,22.3245}, {90.0532,22.315}, {90.0499,22.3147}, {90.0422,22.3177}, {90.0393,22.3049}, {90.0329,22.3039}, {90.0304,22.2979}, {90.0241,22.2934}, {90.0241,22.2878}, {90.0202,22.2753}, {90.0292,22.2597}, {90.0208,22.2417}, {90.0187,22.2305}, {90.0109,22.2286}, {89.9978,22.2287}, {89.9979,22.22}, {90.0005,22.2138}, {89.9996,22.2123}, {89.9945,22.2124}, {89.9916,22.2097}, {89.992,22.2004}, {89.9908,22.1987}, {89.9836,22.2064}, {89.9788,22.2026}, {89.975,22.2039}, {89.9688,22.2113}, {89.9685,22.2206}, {89.9669,22.2202}, {89.9647,22.217}, {89.9639,22.2075}, {89.9693,22.1917}, {89.9588,22.1827}, {89.9577,22.1716}, {89.9555,22.171}, {89.9419,22.1763}, {89.935,22.166}, {89.9301,22.1622}}}
    };

    for (auto& d : districts) {
        District district;
        district.name = d.name;
        district.division = d.division;
        district.divisionIdx = getDivisionIndex(d.division);
        district.extrudeHeight = 0.6f;

        for (auto& pt : d.points) {
            district.polygon.push_back({ (float)pt.first, (float)pt.second });
        }

        m_districts.push_back(std::move(district));
    }

    std::cout << "[MapData] Built " << m_districts.size() << " districts." << std::endl;
}

void MapData::projectCoordinates() {
    float lonMin = 1e9f, lonMax = -1e9f, latMin = 1e9f, latMax = -1e9f;
    for (auto& d : m_districts)
        for (auto& p : d.polygon) {
            lonMin = (std::min)(lonMin, p.x); lonMax = (std::max)(lonMax, p.x);
            latMin = (std::min)(latMin, p.y); latMax = (std::max)(latMax, p.y);
        }

    float ws = 20.f, lr = lonMax - lonMin, latr = latMax - latMin;
    float sc = ws / (std::max)(lr, latr);
    m_worldMin = { 1e9f,1e9f }; m_worldMax = { -1e9f,-1e9f };

    for (auto& d : m_districts)
        for (auto& p : d.polygon) {
            glm::vec2 wp = { (p.x - lonMin) * sc - ws * lr / (std::max)(lr,latr) * 0.5f,
                          (latMax - p.y) * sc - ws * latr / (std::max)(lr,latr) * 0.5f };
            p = wp;
            m_worldMin = { (std::min)(m_worldMin.x,wp.x),(std::min)(m_worldMin.y,wp.y) };
            m_worldMax = { (std::max)(m_worldMax.x,wp.x),(std::max)(m_worldMax.y,wp.y) };
        }

    std::cout << "[MapData] Projected coordinates. World bounds: ["
        << m_worldMin.x << "," << m_worldMin.y << "] to ["
        << m_worldMax.x << "," << m_worldMax.y << "]" << std::endl;
}

void MapData::computeCentroids() {
    for (auto& d : m_districts) {
        glm::vec2 c{ 0.f,0.f };
        for (auto& p : d.polygon) c += p;
        d.centroid = c / (float)d.polygon.size();
    }
}

void MapData::buildGPUBuffers() {
    for (auto& d : m_districts) {
        if (d.polygon.size() < 3) {
            std::cerr << "[MapData] Warning: District " << d.name << " has invalid polygon with "
                << d.polygon.size() << " points, skipping..." << std::endl;
            continue;
        }

        auto idx = earcutPoly(d.polygon);
        if (idx.empty()) {
            std::cerr << "[MapData] Warning: Failed to triangulate polygon for " << d.name << std::endl;
            continue;
        }

        d.vertexCount = (int)idx.size();
        std::vector<float> tv;
        for (auto i : idx) {
            auto& p = d.polygon[i];
            tv.insert(tv.end(), { p.x,d.extrudeHeight,p.y,0.f,1.f,0.f,p.x * 0.1f,p.y * 0.1f });
        }
        glGenVertexArrays(1, &d.vao); glGenBuffers(1, &d.vbo);
        glBindVertexArray(d.vao); glBindBuffer(GL_ARRAY_BUFFER, d.vbo);
        glBufferData(GL_ARRAY_BUFFER, tv.size() * sizeof(float), tv.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));glEnableVertexAttribArray(2);
        int n = (int)d.polygon.size();
        std::vector<float> sv;
        for (int i = 0;i < n;++i) {
            glm::vec2 a = d.polygon[i], b = d.polygon[(i + 1) % n], e = b - a;
            glm::vec3 nm = glm::normalize(glm::vec3(e.y, 0.f, -e.x));
            auto av = [&](float px, float py, float pz) {sv.insert(sv.end(), { px,py,pz,nm.x,nm.y,nm.z,0.f,py });};
            av(a.x, d.extrudeHeight, a.y);av(a.x, 0.f, a.y);av(b.x, d.extrudeHeight, b.y);
            av(b.x, d.extrudeHeight, b.y);av(a.x, 0.f, a.y);av(b.x, 0.f, b.y);
        }
        d.sideVertCount = (int)sv.size() / 8;
        glGenVertexArrays(1, &d.sideVao); glGenBuffers(1, &d.sideVbo);
        glBindVertexArray(d.sideVao); glBindBuffer(GL_ARRAY_BUFFER, d.sideVbo);
        glBufferData(GL_ARRAY_BUFFER, sv.size() * sizeof(float), sv.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));glEnableVertexAttribArray(2);
        std::vector<float> wv;
        for (int i = 0;i < n;++i) {
            glm::vec2 a = d.polygon[i], b = d.polygon[(i + 1) % n];
            wv.insert(wv.end(), { a.x,d.extrudeHeight,a.y,b.x,d.extrudeHeight,b.y });
        }
        d.wireVertCount = (int)wv.size() / 3;
        glGenVertexArrays(1, &d.wireVao); glGenBuffers(1, &d.wireVbo);
        glBindVertexArray(d.wireVao); glBindBuffer(GL_ARRAY_BUFFER, d.wireVbo);
        glBufferData(GL_ARRAY_BUFFER, wv.size() * sizeof(float), wv.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    }
    glBindVertexArray(0);
}

void MapData::update(float dt, float totalTime) {
    for (auto& d : m_districts) {
        float wave = std::sin(totalTime * 0.6f + d.phaseOffset) * 0.08f;
        float target = d.selected ? 1.8f + wave * 0.3f : d.hovered ? 1.2f + wave * 0.5f : wave;
        d.currentY += (target - d.currentY) * (std::min)(1.0f, 8.0f * dt);
        d.targetY = target;
    }
}

District* MapData::hoveredDistrict() { return m_hoveredIdx < 0 ? nullptr : &m_districts[m_hoveredIdx]; }
District* MapData::selectedDistrict() { return m_selectedIdx < 0 ? nullptr : &m_districts[m_selectedIdx]; }
const District* MapData::hoveredDistrict()  const { return m_hoveredIdx < 0 ? nullptr : &m_districts[m_hoveredIdx]; }
const District* MapData::selectedDistrict() const { return m_selectedIdx < 0 ? nullptr : &m_districts[m_selectedIdx]; }

void MapData::setHovered(int idx) {
    if (m_hoveredIdx >= 0) m_districts[m_hoveredIdx].hovered = false;
    m_hoveredIdx = idx;
    if (idx >= 0) m_districts[idx].hovered = true;
}

void MapData::setSelected(int idx) {
    if (m_selectedIdx >= 0) m_districts[m_selectedIdx].selected = false;
    m_selectedIdx = idx;
    if (idx >= 0) m_districts[idx].selected = true;
}