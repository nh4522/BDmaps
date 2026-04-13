#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

class MapData;
class Camera;

class UIOverlay {
public:
    UIOverlay(GLFWwindow* window, int width, int height);
    ~UIOverlay();

    void draw(const MapData& mapData, const Camera& camera, int width, int height);
    // Add to UIOverlay.h
    int getSelectedSearchResult() const { return m_selectedSearchResult; }
    void clearSelectedSearchResult() { m_selectedSearchResult = -1; }

private:
    void drawInfoPanel(const MapData& mapData);
    void drawLegend();
    void drawControls();
    void drawTitle();
    void drawSearchBar(const MapData& mapData);
    void drawDistrictLabelsInternal(const MapData& mapData, const Camera& camera, int width, int height);

    std::string m_searchQuery;
    int m_selectedSearchResult = -1;
    std::vector<int> m_searchResults;
};