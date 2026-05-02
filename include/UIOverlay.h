#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <map>

class MapData;
class Camera;

class UIOverlay {
public:
    UIOverlay(GLFWwindow* window, int width, int height);
    ~UIOverlay();

    void draw(const MapData& mapData, const Camera& camera, int width, int height);

    int getSelectedSearchResult() const { return m_selectedSearchResult; }
    void clearSelectedSearchResult() { m_selectedSearchResult = -1; }

private:
    void drawInfoPanel(const MapData& mapData);
    void drawLegend();
    void drawControls();
    void drawTitle();
    void drawSearchBar(const MapData& mapData);
    void drawDivisionDropdown(const MapData& mapData);
    void drawDistrictLabelsInternal(const MapData& mapData, const Camera& camera, int width, int height);
    void updateSearchSuggestions(const MapData& mapData);

    std::string m_searchQuery;
    int m_selectedSearchResult = -1;
    std::vector<int> m_searchResults;
    std::vector<std::string> m_searchSuggestions;

    // Division dropdown state
    int m_selectedDivision = -1;
    bool m_showDistrictDropdown = false;
};