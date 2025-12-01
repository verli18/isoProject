#include "../include/visualSettings.hpp"
#include "../include/worldGenerator.hpp"
#include "../include/worldMap.hpp"
#include <fstream>
#include <sstream>
#include <map>
#include <cstdio>

void VisualSettings::initialize() {
    if (initialized) return;
    
    // Try to load default settings file first
    if (!loadFromFile(DEFAULT_SETTINGS_FILE)) {
        // Try legacy file name
        if (!loadFromFile("visual_settings.ini")) {
            TraceLog(LOG_INFO, "No settings file found, using defaults");
        } else {
            TraceLog(LOG_INFO, "Loaded settings from visual_settings.ini");
        }
    } else {
        TraceLog(LOG_INFO, "Loaded default settings from %s", DEFAULT_SETTINGS_FILE);
    }
    
    dirty = true;
    initialized = true;
}

void VisualSettings::resetToDefaults() {
    grass = GrassSettings{};
    water = WaterSettings{};
    terrain = TerrainSettings{};
    lighting = LightingSettings{};
    dirty = true;
}

bool VisualSettings::saveToFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open %s for writing", filename.c_str());
        return false;
    }
    
    file << "# Visual Settings Configuration\n";
    file << "# Auto-generated - edit with care\n\n";
    
    // Grass settings
    file << "[Grass]\n";
    file << "tipColor = " << grass.tipColor.x << ", " << grass.tipColor.y << ", " << grass.tipColor.z << "\n";
    file << "baseColor = " << grass.baseColor.x << ", " << grass.baseColor.y << ", " << grass.baseColor.z << "\n";
    file << "tundraTipColor = " << grass.tundraTipColor.x << ", " << grass.tundraTipColor.y << ", " << grass.tundraTipColor.z << "\n";
    file << "tundraBaseColor = " << grass.tundraBaseColor.x << ", " << grass.tundraBaseColor.y << ", " << grass.tundraBaseColor.z << "\n";
    file << "snowTipColor = " << grass.snowTipColor.x << ", " << grass.snowTipColor.y << ", " << grass.snowTipColor.z << "\n";
    file << "snowBaseColor = " << grass.snowBaseColor.x << ", " << grass.snowBaseColor.y << ", " << grass.snowBaseColor.z << "\n";
    file << "tundraStartTemp = " << grass.tundraStartTemp << "\n";
    file << "tundraFullTemp = " << grass.tundraFullTemp << "\n";
    file << "snowStartTemp = " << grass.snowStartTemp << "\n";
    file << "snowFullTemp = " << grass.snowFullTemp << "\n";
    file << "noGrassTemp = " << grass.noGrassTemp << "\n";
    file << "dirtBlendColor = " << grass.dirtBlendColor.x << ", " << grass.dirtBlendColor.y << ", " << grass.dirtBlendColor.z << "\n";
    file << "dirtBlendDistance = " << grass.dirtBlendDistance << "\n";
    file << "dirtBlendStrength = " << grass.dirtBlendStrength << "\n";
    file << "temperatureInfluence = " << grass.temperatureInfluence << "\n";
    file << "moistureInfluence = " << grass.moistureInfluence << "\n";
    file << "biologicalInfluence = " << grass.biologicalInfluence << "\n";
    file << "windStrength = " << grass.windStrength << "\n";
    file << "windSpeed = " << grass.windSpeed << "\n";
    file << "windDirection = " << grass.windDirection.x << ", " << grass.windDirection.y << "\n";
    file << "baseHeight = " << grass.baseHeight << "\n";
    file << "heightVariation = " << grass.heightVariation << "\n";
    file << "bladeWidth = " << grass.bladeWidth << "\n";
    file << "bladeTaper = " << grass.bladeTaper << "\n";
    file << "tundraHeightMult = " << grass.tundraHeightMult << "\n";
    file << "snowHeightMult = " << grass.snowHeightMult << "\n";
    file << "bladesPerTile = " << grass.bladesPerTile << "\n";
    file << "moistureMultiplier = " << grass.moistureMultiplier << "\n";
    file << "slopeReduction = " << grass.slopeReduction << "\n";
    file << "minDensity = " << grass.minDensity << "\n";
    file << "tundraDensityMult = " << grass.tundraDensityMult << "\n";
    file << "snowDensityMult = " << grass.snowDensityMult << "\n";
    file << "renderDistance = " << grass.renderDistance << "\n";
    file << "fadeStartDistance = " << grass.fadeStartDistance << "\n";
    file << "lodLevels = " << grass.lodLevels << "\n";
    file << "lodReduction = " << grass.lodReduction << "\n";
    file << "\n";
    
    // Water settings
    file << "[Water]\n";
    file << "hueShift = " << water.hueShift << "\n";
    file << "saturationMult = " << water.saturationMult << "\n";
    file << "valueMult = " << water.valueMult << "\n";
    file << "minDepth = " << water.minDepth << "\n";
    file << "maxDepth = " << water.maxDepth << "\n";
    file << "minAlpha = " << water.minAlpha << "\n";
    file << "maxAlpha = " << water.maxAlpha << "\n";
    file << "shallowColor = " << water.shallowColor.x << ", " << water.shallowColor.y << ", " << water.shallowColor.z << "\n";
    file << "deepColor = " << water.deepColor.x << ", " << water.deepColor.y << ", " << water.deepColor.z << "\n";
    file << "flowSpeed = " << water.flowSpeed << "\n";
    file << "rippleSpeed = " << water.rippleSpeed << "\n";
    file << "displacementIntensity = " << water.displacementIntensity << "\n";
    file << "\n";
    
    // Terrain settings
    file << "[Terrain]\n";
    file << "colorSaturation = " << terrain.colorSaturation << "\n";
    file << "colorBrightness = " << terrain.colorBrightness << "\n";
    file << "erosionThreshold = " << terrain.erosionThreshold << "\n";
    file << "erosionFullExpose = " << terrain.erosionFullExpose << "\n";
    file << "grassExposedU = " << terrain.grassExposedU << "\n";
    file << "snowExposedU = " << terrain.snowExposedU << "\n";
    file << "sandExposedU = " << terrain.sandExposedU << "\n";
    file << "stoneExposedU = " << terrain.stoneExposedU << "\n";
    file << "ditherIntensity = " << terrain.ditherIntensity << "\n";
    file << "visualizationMode = " << terrain.visualizationMode << "\n";
    file << "tundraTextureU = " << terrain.tundraTextureU << "\n";
    file << "snowTextureU = " << terrain.snowTextureU << "\n";
    file << "\n";
    
    // Lighting settings
    file << "[Lighting]\n";
    file << "sunDirection = " << lighting.sunDirection.x << ", " << lighting.sunDirection.y << ", " << lighting.sunDirection.z << "\n";
    file << "sunColor = " << lighting.sunColor.x << ", " << lighting.sunColor.y << ", " << lighting.sunColor.z << "\n";
    file << "ambientStrength = " << lighting.ambientStrength << "\n";
    file << "ambientColor = " << lighting.ambientColor.x << ", " << lighting.ambientColor.y << ", " << lighting.ambientColor.z << "\n";
    file << "shiftIntensity = " << lighting.shiftIntensity << "\n";
    file << "shiftDisplacement = " << lighting.shiftDisplacement << "\n";
    
    file.close();
    TraceLog(LOG_INFO, "Saved visual settings to %s", filename.c_str());
    return true;
}

// Helper to parse a Vector3 from "x, y, z" format
static bool parseVector3(const std::string& value, Vector3& out) {
    float x, y, z;
    if (sscanf(value.c_str(), "%f, %f, %f", &x, &y, &z) == 3) {
        out = {x, y, z};
        return true;
    }
    return false;
}

// Helper to parse a Vector2 from "x, y" format
static bool parseVector2(const std::string& value, Vector2& out) {
    float x, y;
    if (sscanf(value.c_str(), "%f, %f", &x, &y) == 2) {
        out = {x, y};
        return true;
    }
    return false;
}

bool VisualSettings::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line, currentSection;
    std::map<std::string, std::map<std::string, std::string>> sections;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        // Check for section header
        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end != std::string::npos) {
                currentSection = line.substr(1, end - 1);
            }
            continue;
        }
        
        // Parse key = value
        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            
            // Trim whitespace
            while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
            while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.erase(0, 1);
            
            sections[currentSection][key] = value;
        }
    }
    file.close();
    
    // Apply Grass settings
    auto& g = sections["Grass"];
    if (!g.empty()) {
        parseVector3(g["tipColor"], grass.tipColor);
        parseVector3(g["baseColor"], grass.baseColor);
        parseVector3(g["tundraTipColor"], grass.tundraTipColor);
        parseVector3(g["tundraBaseColor"], grass.tundraBaseColor);
        parseVector3(g["snowTipColor"], grass.snowTipColor);
        parseVector3(g["snowBaseColor"], grass.snowBaseColor);
        if (!g["tundraStartTemp"].empty()) grass.tundraStartTemp = std::stof(g["tundraStartTemp"]);
        if (!g["tundraFullTemp"].empty()) grass.tundraFullTemp = std::stof(g["tundraFullTemp"]);
        if (!g["snowStartTemp"].empty()) grass.snowStartTemp = std::stof(g["snowStartTemp"]);
        if (!g["snowFullTemp"].empty()) grass.snowFullTemp = std::stof(g["snowFullTemp"]);
        if (!g["noGrassTemp"].empty()) grass.noGrassTemp = std::stof(g["noGrassTemp"]);
        parseVector3(g["dirtBlendColor"], grass.dirtBlendColor);
        if (!g["dirtBlendDistance"].empty()) grass.dirtBlendDistance = std::stof(g["dirtBlendDistance"]);
        if (!g["dirtBlendStrength"].empty()) grass.dirtBlendStrength = std::stof(g["dirtBlendStrength"]);
        if (!g["temperatureInfluence"].empty()) grass.temperatureInfluence = std::stof(g["temperatureInfluence"]);
        if (!g["moistureInfluence"].empty()) grass.moistureInfluence = std::stof(g["moistureInfluence"]);
        if (!g["biologicalInfluence"].empty()) grass.biologicalInfluence = std::stof(g["biologicalInfluence"]);
        if (!g["windStrength"].empty()) grass.windStrength = std::stof(g["windStrength"]);
        if (!g["windSpeed"].empty()) grass.windSpeed = std::stof(g["windSpeed"]);
        parseVector2(g["windDirection"], grass.windDirection);
        if (!g["baseHeight"].empty()) grass.baseHeight = std::stof(g["baseHeight"]);
        if (!g["heightVariation"].empty()) grass.heightVariation = std::stof(g["heightVariation"]);
        if (!g["bladeWidth"].empty()) grass.bladeWidth = std::stof(g["bladeWidth"]);
        if (!g["bladeTaper"].empty()) grass.bladeTaper = std::stof(g["bladeTaper"]);
        if (!g["tundraHeightMult"].empty()) grass.tundraHeightMult = std::stof(g["tundraHeightMult"]);
        if (!g["snowHeightMult"].empty()) grass.snowHeightMult = std::stof(g["snowHeightMult"]);
        if (!g["bladesPerTile"].empty()) grass.bladesPerTile = std::stof(g["bladesPerTile"]);
        if (!g["moistureMultiplier"].empty()) grass.moistureMultiplier = std::stof(g["moistureMultiplier"]);
        if (!g["slopeReduction"].empty()) grass.slopeReduction = std::stof(g["slopeReduction"]);
        if (!g["minDensity"].empty()) grass.minDensity = std::stof(g["minDensity"]);
        if (!g["tundraDensityMult"].empty()) grass.tundraDensityMult = std::stof(g["tundraDensityMult"]);
        if (!g["snowDensityMult"].empty()) grass.snowDensityMult = std::stof(g["snowDensityMult"]);
        if (!g["renderDistance"].empty()) grass.renderDistance = std::stof(g["renderDistance"]);
        if (!g["fadeStartDistance"].empty()) grass.fadeStartDistance = std::stof(g["fadeStartDistance"]);
        if (!g["lodLevels"].empty()) grass.lodLevels = std::stoi(g["lodLevels"]);
        if (!g["lodReduction"].empty()) grass.lodReduction = std::stof(g["lodReduction"]);
    }
    
    // Apply Water settings
    auto& w = sections["Water"];
    if (!w.empty()) {
        if (!w["hueShift"].empty()) water.hueShift = std::stof(w["hueShift"]);
        if (!w["saturationMult"].empty()) water.saturationMult = std::stof(w["saturationMult"]);
        if (!w["valueMult"].empty()) water.valueMult = std::stof(w["valueMult"]);
        if (!w["minDepth"].empty()) water.minDepth = std::stof(w["minDepth"]);
        if (!w["maxDepth"].empty()) water.maxDepth = std::stof(w["maxDepth"]);
        if (!w["minAlpha"].empty()) water.minAlpha = std::stof(w["minAlpha"]);
        if (!w["maxAlpha"].empty()) water.maxAlpha = std::stof(w["maxAlpha"]);
        parseVector3(w["shallowColor"], water.shallowColor);
        parseVector3(w["deepColor"], water.deepColor);
        if (!w["flowSpeed"].empty()) water.flowSpeed = std::stof(w["flowSpeed"]);
        if (!w["rippleSpeed"].empty()) water.rippleSpeed = std::stof(w["rippleSpeed"]);
        if (!w["displacementIntensity"].empty()) water.displacementIntensity = std::stof(w["displacementIntensity"]);
    }
    
    // Apply Terrain settings
    auto& t = sections["Terrain"];
    if (!t.empty()) {
        if (!t["colorSaturation"].empty()) terrain.colorSaturation = std::stof(t["colorSaturation"]);
        if (!t["colorBrightness"].empty()) terrain.colorBrightness = std::stof(t["colorBrightness"]);
        if (!t["erosionThreshold"].empty()) terrain.erosionThreshold = std::stof(t["erosionThreshold"]);
        if (!t["erosionFullExpose"].empty()) terrain.erosionFullExpose = std::stof(t["erosionFullExpose"]);
        if (!t["grassExposedU"].empty()) terrain.grassExposedU = std::stoi(t["grassExposedU"]);
        if (!t["snowExposedU"].empty()) terrain.snowExposedU = std::stoi(t["snowExposedU"]);
        if (!t["sandExposedU"].empty()) terrain.sandExposedU = std::stoi(t["sandExposedU"]);
        if (!t["stoneExposedU"].empty()) terrain.stoneExposedU = std::stoi(t["stoneExposedU"]);
        if (!t["ditherIntensity"].empty()) terrain.ditherIntensity = std::stof(t["ditherIntensity"]);
        if (!t["visualizationMode"].empty()) terrain.visualizationMode = std::stoi(t["visualizationMode"]);
    }
    
    // Apply Lighting settings
    auto& l = sections["Lighting"];
    if (!l.empty()) {
        parseVector3(l["sunDirection"], lighting.sunDirection);
        parseVector3(l["sunColor"], lighting.sunColor);
        if (!l["ambientStrength"].empty()) lighting.ambientStrength = std::stof(l["ambientStrength"]);
        parseVector3(l["ambientColor"], lighting.ambientColor);
        if (!l["shiftIntensity"].empty()) lighting.shiftIntensity = std::stof(l["shiftIntensity"]);
        if (!l["shiftDisplacement"].empty()) lighting.shiftDisplacement = std::stof(l["shiftDisplacement"]);
    }
    
    dirty = true;
    return true;
}

bool VisualSettings::saveAllSettings(const std::string& filename,
                                      const WorldGenConfig* worldGen,
                                      const ErosionConfig* erosion) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        TraceLog(LOG_ERROR, "Failed to open %s for writing", filename.c_str());
        return false;
    }
    
    file << "# Complete Settings Configuration\n";
    file << "# Auto-generated - edit with care\n\n";
    
    // ========== Visual Settings ==========
    
    // Grass settings
    file << "[Grass]\n";
    file << "tipColor = " << grass.tipColor.x << ", " << grass.tipColor.y << ", " << grass.tipColor.z << "\n";
    file << "baseColor = " << grass.baseColor.x << ", " << grass.baseColor.y << ", " << grass.baseColor.z << "\n";
    file << "tundraTipColor = " << grass.tundraTipColor.x << ", " << grass.tundraTipColor.y << ", " << grass.tundraTipColor.z << "\n";
    file << "tundraBaseColor = " << grass.tundraBaseColor.x << ", " << grass.tundraBaseColor.y << ", " << grass.tundraBaseColor.z << "\n";
    file << "snowTipColor = " << grass.snowTipColor.x << ", " << grass.snowTipColor.y << ", " << grass.snowTipColor.z << "\n";
    file << "snowBaseColor = " << grass.snowBaseColor.x << ", " << grass.snowBaseColor.y << ", " << grass.snowBaseColor.z << "\n";
    file << "tundraStartTemp = " << grass.tundraStartTemp << "\n";
    file << "tundraFullTemp = " << grass.tundraFullTemp << "\n";
    file << "snowStartTemp = " << grass.snowStartTemp << "\n";
    file << "snowFullTemp = " << grass.snowFullTemp << "\n";
    file << "noGrassTemp = " << grass.noGrassTemp << "\n";
    file << "dirtBlendColor = " << grass.dirtBlendColor.x << ", " << grass.dirtBlendColor.y << ", " << grass.dirtBlendColor.z << "\n";
    file << "dirtBlendDistance = " << grass.dirtBlendDistance << "\n";
    file << "dirtBlendStrength = " << grass.dirtBlendStrength << "\n";
    file << "temperatureInfluence = " << grass.temperatureInfluence << "\n";
    file << "moistureInfluence = " << grass.moistureInfluence << "\n";
    file << "biologicalInfluence = " << grass.biologicalInfluence << "\n";
    file << "windStrength = " << grass.windStrength << "\n";
    file << "windSpeed = " << grass.windSpeed << "\n";
    file << "windDirection = " << grass.windDirection.x << ", " << grass.windDirection.y << "\n";
    file << "baseHeight = " << grass.baseHeight << "\n";
    file << "heightVariation = " << grass.heightVariation << "\n";
    file << "bladeWidth = " << grass.bladeWidth << "\n";
    file << "bladeTaper = " << grass.bladeTaper << "\n";
    file << "tundraHeightMult = " << grass.tundraHeightMult << "\n";
    file << "snowHeightMult = " << grass.snowHeightMult << "\n";
    file << "bladesPerTile = " << grass.bladesPerTile << "\n";
    file << "moistureMultiplier = " << grass.moistureMultiplier << "\n";
    file << "slopeReduction = " << grass.slopeReduction << "\n";
    file << "minDensity = " << grass.minDensity << "\n";
    file << "tundraDensityMult = " << grass.tundraDensityMult << "\n";
    file << "snowDensityMult = " << grass.snowDensityMult << "\n";
    file << "renderDistance = " << grass.renderDistance << "\n";
    file << "fadeStartDistance = " << grass.fadeStartDistance << "\n";
    file << "lodLevels = " << grass.lodLevels << "\n";
    file << "lodReduction = " << grass.lodReduction << "\n";
    file << "\n";
    
    // Water settings
    file << "[Water]\n";
    file << "hueShift = " << water.hueShift << "\n";
    file << "saturationMult = " << water.saturationMult << "\n";
    file << "valueMult = " << water.valueMult << "\n";
    file << "minDepth = " << water.minDepth << "\n";
    file << "maxDepth = " << water.maxDepth << "\n";
    file << "minAlpha = " << water.minAlpha << "\n";
    file << "maxAlpha = " << water.maxAlpha << "\n";
    file << "shallowColor = " << water.shallowColor.x << ", " << water.shallowColor.y << ", " << water.shallowColor.z << "\n";
    file << "deepColor = " << water.deepColor.x << ", " << water.deepColor.y << ", " << water.deepColor.z << "\n";
    file << "flowSpeed = " << water.flowSpeed << "\n";
    file << "rippleSpeed = " << water.rippleSpeed << "\n";
    file << "displacementIntensity = " << water.displacementIntensity << "\n";
    file << "\n";
    
    // Terrain settings
    file << "[Terrain]\n";
    file << "colorSaturation = " << terrain.colorSaturation << "\n";
    file << "colorBrightness = " << terrain.colorBrightness << "\n";
    file << "erosionThreshold = " << terrain.erosionThreshold << "\n";
    file << "erosionFullExpose = " << terrain.erosionFullExpose << "\n";
    file << "grassExposedU = " << terrain.grassExposedU << "\n";
    file << "snowExposedU = " << terrain.snowExposedU << "\n";
    file << "sandExposedU = " << terrain.sandExposedU << "\n";
    file << "stoneExposedU = " << terrain.stoneExposedU << "\n";
    file << "ditherIntensity = " << terrain.ditherIntensity << "\n";
    file << "visualizationMode = " << terrain.visualizationMode << "\n";
    file << "tundraTextureU = " << terrain.tundraTextureU << "\n";
    file << "snowTextureU = " << terrain.snowTextureU << "\n";
    file << "\n";
    
    // Lighting settings
    file << "[Lighting]\n";
    file << "sunDirection = " << lighting.sunDirection.x << ", " << lighting.sunDirection.y << ", " << lighting.sunDirection.z << "\n";
    file << "sunColor = " << lighting.sunColor.x << ", " << lighting.sunColor.y << ", " << lighting.sunColor.z << "\n";
    file << "ambientStrength = " << lighting.ambientStrength << "\n";
    file << "ambientColor = " << lighting.ambientColor.x << ", " << lighting.ambientColor.y << ", " << lighting.ambientColor.z << "\n";
    file << "shiftIntensity = " << lighting.shiftIntensity << "\n";
    file << "shiftDisplacement = " << lighting.shiftDisplacement << "\n";
    file << "\n";
    
    // ========== World Generation Settings ==========
    
    if (worldGen) {
        file << "[WorldGeneration]\n";
        file << "seed = " << worldGen->seed << "\n";
        file << "heightScale = " << worldGen->heightScale << "\n";
        file << "heightBase = " << worldGen->heightBase << "\n";
        file << "heightExponent = " << worldGen->heightExponent << "\n";
        file << "terrainFreq = " << worldGen->terrainFreq << "\n";
        file << "regionFreq = " << worldGen->regionFreq << "\n";
        file << "potentialFreq = " << worldGen->potentialFreq << "\n";
        file << "climateFreq = " << worldGen->climateFreq << "\n";
        file << "warpAmplitude = " << worldGen->warpAmplitude << "\n";
        file << "warpFrequency = " << worldGen->warpFrequency << "\n";
        file << "geologicalOverrideThreshold = " << worldGen->geologicalOverrideThreshold << "\n";
        file << "seaLevel = " << worldGen->seaLevel << "\n";
        file << "slopeToSulfide = " << worldGen->slopeToSulfide << "\n";
        file << "slopeToCrystalline = " << worldGen->slopeToCrystalline << "\n";
        file << "flowToBiological = " << worldGen->flowToBiological << "\n";
        file << "flowToHydrological = " << worldGen->flowToHydrological << "\n";
        file << "\n";
    }
    
    // ========== Erosion Settings ==========
    
    if (erosion) {
        file << "[Erosion]\n";
        file << "numDroplets = " << erosion->numDroplets << "\n";
        file << "maxDropletLifetime = " << erosion->maxDropletLifetime << "\n";
        file << "inertia = " << erosion->inertia << "\n";
        file << "sedimentCapacity = " << erosion->sedimentCapacity << "\n";
        file << "minSedimentCapacity = " << erosion->minSedimentCapacity << "\n";
        file << "erodeSpeed = " << erosion->erodeSpeed << "\n";
        file << "depositSpeed = " << erosion->depositSpeed << "\n";
        file << "evaporateSpeed = " << erosion->evaporateSpeed << "\n";
        file << "gravity = " << erosion->gravity << "\n";
        file << "maxErodePerStep = " << erosion->maxErodePerStep << "\n";
        file << "erosionRadius = " << erosion->erosionRadius << "\n";
        file << "waterMinDepth = " << erosion->waterMinDepth << "\n";
        file << "lakeDilation = " << erosion->lakeDilation << "\n";
        file << "riverFlowThreshold = " << erosion->riverFlowThreshold << "\n";
        file << "riverWidthScale = " << erosion->riverWidthScale << "\n";
        file << "maxRiverWidth = " << erosion->maxRiverWidth << "\n";
        file << "riverDepth = " << erosion->riverDepth << "\n";
        file << "\n";
    }
    
    file.close();
    TraceLog(LOG_INFO, "Saved all settings to %s", filename.c_str());
    return true;
}

bool VisualSettings::loadAllSettings(const std::string& filename,
                                      WorldGenConfig* worldGen,
                                      ErosionConfig* erosion) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line, currentSection;
    std::map<std::string, std::map<std::string, std::string>> sections;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        // Check for section header
        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end != std::string::npos) {
                currentSection = line.substr(1, end - 1);
            }
            continue;
        }
        
        // Parse key = value
        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            
            // Trim whitespace
            while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
            while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.erase(0, 1);
            
            sections[currentSection][key] = value;
        }
    }
    file.close();
    
    // ========== Visual Settings ==========
    
    // Apply Grass settings
    auto& g = sections["Grass"];
    if (!g.empty()) {
        parseVector3(g["tipColor"], grass.tipColor);
        parseVector3(g["baseColor"], grass.baseColor);
        parseVector3(g["tundraTipColor"], grass.tundraTipColor);
        parseVector3(g["tundraBaseColor"], grass.tundraBaseColor);
        parseVector3(g["snowTipColor"], grass.snowTipColor);
        parseVector3(g["snowBaseColor"], grass.snowBaseColor);
        if (!g["tundraStartTemp"].empty()) grass.tundraStartTemp = std::stof(g["tundraStartTemp"]);
        if (!g["tundraFullTemp"].empty()) grass.tundraFullTemp = std::stof(g["tundraFullTemp"]);
        if (!g["snowStartTemp"].empty()) grass.snowStartTemp = std::stof(g["snowStartTemp"]);
        if (!g["snowFullTemp"].empty()) grass.snowFullTemp = std::stof(g["snowFullTemp"]);
        if (!g["noGrassTemp"].empty()) grass.noGrassTemp = std::stof(g["noGrassTemp"]);
        parseVector3(g["dirtBlendColor"], grass.dirtBlendColor);
        if (!g["dirtBlendDistance"].empty()) grass.dirtBlendDistance = std::stof(g["dirtBlendDistance"]);
        if (!g["dirtBlendStrength"].empty()) grass.dirtBlendStrength = std::stof(g["dirtBlendStrength"]);
        if (!g["temperatureInfluence"].empty()) grass.temperatureInfluence = std::stof(g["temperatureInfluence"]);
        if (!g["moistureInfluence"].empty()) grass.moistureInfluence = std::stof(g["moistureInfluence"]);
        if (!g["biologicalInfluence"].empty()) grass.biologicalInfluence = std::stof(g["biologicalInfluence"]);
        if (!g["windStrength"].empty()) grass.windStrength = std::stof(g["windStrength"]);
        if (!g["windSpeed"].empty()) grass.windSpeed = std::stof(g["windSpeed"]);
        parseVector2(g["windDirection"], grass.windDirection);
        if (!g["baseHeight"].empty()) grass.baseHeight = std::stof(g["baseHeight"]);
        if (!g["heightVariation"].empty()) grass.heightVariation = std::stof(g["heightVariation"]);
        if (!g["bladeWidth"].empty()) grass.bladeWidth = std::stof(g["bladeWidth"]);
        if (!g["bladeTaper"].empty()) grass.bladeTaper = std::stof(g["bladeTaper"]);
        if (!g["tundraHeightMult"].empty()) grass.tundraHeightMult = std::stof(g["tundraHeightMult"]);
        if (!g["snowHeightMult"].empty()) grass.snowHeightMult = std::stof(g["snowHeightMult"]);
        if (!g["bladesPerTile"].empty()) grass.bladesPerTile = std::stof(g["bladesPerTile"]);
        if (!g["moistureMultiplier"].empty()) grass.moistureMultiplier = std::stof(g["moistureMultiplier"]);
        if (!g["slopeReduction"].empty()) grass.slopeReduction = std::stof(g["slopeReduction"]);
        if (!g["minDensity"].empty()) grass.minDensity = std::stof(g["minDensity"]);
        if (!g["tundraDensityMult"].empty()) grass.tundraDensityMult = std::stof(g["tundraDensityMult"]);
        if (!g["snowDensityMult"].empty()) grass.snowDensityMult = std::stof(g["snowDensityMult"]);
        if (!g["renderDistance"].empty()) grass.renderDistance = std::stof(g["renderDistance"]);
        if (!g["fadeStartDistance"].empty()) grass.fadeStartDistance = std::stof(g["fadeStartDistance"]);
        if (!g["lodLevels"].empty()) grass.lodLevels = std::stoi(g["lodLevels"]);
        if (!g["lodReduction"].empty()) grass.lodReduction = std::stof(g["lodReduction"]);
    }
    
    // Apply Water settings
    auto& w = sections["Water"];
    if (!w.empty()) {
        if (!w["hueShift"].empty()) water.hueShift = std::stof(w["hueShift"]);
        if (!w["saturationMult"].empty()) water.saturationMult = std::stof(w["saturationMult"]);
        if (!w["valueMult"].empty()) water.valueMult = std::stof(w["valueMult"]);
        if (!w["minDepth"].empty()) water.minDepth = std::stof(w["minDepth"]);
        if (!w["maxDepth"].empty()) water.maxDepth = std::stof(w["maxDepth"]);
        if (!w["minAlpha"].empty()) water.minAlpha = std::stof(w["minAlpha"]);
        if (!w["maxAlpha"].empty()) water.maxAlpha = std::stof(w["maxAlpha"]);
        parseVector3(w["shallowColor"], water.shallowColor);
        parseVector3(w["deepColor"], water.deepColor);
        if (!w["flowSpeed"].empty()) water.flowSpeed = std::stof(w["flowSpeed"]);
        if (!w["rippleSpeed"].empty()) water.rippleSpeed = std::stof(w["rippleSpeed"]);
        if (!w["displacementIntensity"].empty()) water.displacementIntensity = std::stof(w["displacementIntensity"]);
    }
    
    // Apply Terrain settings
    auto& t = sections["Terrain"];
    if (!t.empty()) {
        if (!t["colorSaturation"].empty()) terrain.colorSaturation = std::stof(t["colorSaturation"]);
        if (!t["colorBrightness"].empty()) terrain.colorBrightness = std::stof(t["colorBrightness"]);
        if (!t["erosionThreshold"].empty()) terrain.erosionThreshold = std::stof(t["erosionThreshold"]);
        if (!t["erosionFullExpose"].empty()) terrain.erosionFullExpose = std::stof(t["erosionFullExpose"]);
        if (!t["grassExposedU"].empty()) terrain.grassExposedU = std::stoi(t["grassExposedU"]);
        if (!t["snowExposedU"].empty()) terrain.snowExposedU = std::stoi(t["snowExposedU"]);
        if (!t["sandExposedU"].empty()) terrain.sandExposedU = std::stoi(t["sandExposedU"]);
        if (!t["stoneExposedU"].empty()) terrain.stoneExposedU = std::stoi(t["stoneExposedU"]);
        if (!t["ditherIntensity"].empty()) terrain.ditherIntensity = std::stof(t["ditherIntensity"]);
        if (!t["visualizationMode"].empty()) terrain.visualizationMode = std::stoi(t["visualizationMode"]);
        if (!t["tundraTextureU"].empty()) terrain.tundraTextureU = std::stoi(t["tundraTextureU"]);
        if (!t["snowTextureU"].empty()) terrain.snowTextureU = std::stoi(t["snowTextureU"]);
    }
    
    // Apply Lighting settings
    auto& l = sections["Lighting"];
    if (!l.empty()) {
        parseVector3(l["sunDirection"], lighting.sunDirection);
        parseVector3(l["sunColor"], lighting.sunColor);
        if (!l["ambientStrength"].empty()) lighting.ambientStrength = std::stof(l["ambientStrength"]);
        parseVector3(l["ambientColor"], lighting.ambientColor);
        if (!l["shiftIntensity"].empty()) lighting.shiftIntensity = std::stof(l["shiftIntensity"]);
        if (!l["shiftDisplacement"].empty()) lighting.shiftDisplacement = std::stof(l["shiftDisplacement"]);
    }
    
    // ========== World Generation Settings ==========
    
    if (worldGen) {
        auto& wg = sections["WorldGeneration"];
        if (!wg.empty()) {
            if (!wg["seed"].empty()) worldGen->seed = std::stoi(wg["seed"]);
            if (!wg["heightScale"].empty()) worldGen->heightScale = std::stof(wg["heightScale"]);
            if (!wg["heightBase"].empty()) worldGen->heightBase = std::stof(wg["heightBase"]);
            if (!wg["heightExponent"].empty()) worldGen->heightExponent = std::stof(wg["heightExponent"]);
            if (!wg["terrainFreq"].empty()) worldGen->terrainFreq = std::stof(wg["terrainFreq"]);
            if (!wg["regionFreq"].empty()) worldGen->regionFreq = std::stof(wg["regionFreq"]);
            if (!wg["potentialFreq"].empty()) worldGen->potentialFreq = std::stof(wg["potentialFreq"]);
            if (!wg["climateFreq"].empty()) worldGen->climateFreq = std::stof(wg["climateFreq"]);
            if (!wg["warpAmplitude"].empty()) worldGen->warpAmplitude = std::stof(wg["warpAmplitude"]);
            if (!wg["warpFrequency"].empty()) worldGen->warpFrequency = std::stof(wg["warpFrequency"]);
            if (!wg["geologicalOverrideThreshold"].empty()) worldGen->geologicalOverrideThreshold = std::stof(wg["geologicalOverrideThreshold"]);
            if (!wg["seaLevel"].empty()) worldGen->seaLevel = std::stof(wg["seaLevel"]);
            if (!wg["slopeToSulfide"].empty()) worldGen->slopeToSulfide = std::stof(wg["slopeToSulfide"]);
            if (!wg["slopeToCrystalline"].empty()) worldGen->slopeToCrystalline = std::stof(wg["slopeToCrystalline"]);
            if (!wg["flowToBiological"].empty()) worldGen->flowToBiological = std::stof(wg["flowToBiological"]);
            if (!wg["flowToHydrological"].empty()) worldGen->flowToHydrological = std::stof(wg["flowToHydrological"]);
        }
    }
    
    // ========== Erosion Settings ==========
    
    if (erosion) {
        auto& e = sections["Erosion"];
        if (!e.empty()) {
            if (!e["numDroplets"].empty()) erosion->numDroplets = std::stoi(e["numDroplets"]);
            if (!e["maxDropletLifetime"].empty()) erosion->maxDropletLifetime = std::stoi(e["maxDropletLifetime"]);
            if (!e["inertia"].empty()) erosion->inertia = std::stof(e["inertia"]);
            if (!e["sedimentCapacity"].empty()) erosion->sedimentCapacity = std::stof(e["sedimentCapacity"]);
            if (!e["minSedimentCapacity"].empty()) erosion->minSedimentCapacity = std::stof(e["minSedimentCapacity"]);
            if (!e["erodeSpeed"].empty()) erosion->erodeSpeed = std::stof(e["erodeSpeed"]);
            if (!e["depositSpeed"].empty()) erosion->depositSpeed = std::stof(e["depositSpeed"]);
            if (!e["evaporateSpeed"].empty()) erosion->evaporateSpeed = std::stof(e["evaporateSpeed"]);
            if (!e["gravity"].empty()) erosion->gravity = std::stof(e["gravity"]);
            if (!e["maxErodePerStep"].empty()) erosion->maxErodePerStep = std::stof(e["maxErodePerStep"]);
            if (!e["erosionRadius"].empty()) erosion->erosionRadius = std::stoi(e["erosionRadius"]);
            if (!e["waterMinDepth"].empty()) erosion->waterMinDepth = std::stof(e["waterMinDepth"]);
            if (!e["lakeDilation"].empty()) erosion->lakeDilation = std::stoi(e["lakeDilation"]);
            if (!e["riverFlowThreshold"].empty()) erosion->riverFlowThreshold = std::stoi(e["riverFlowThreshold"]);
            if (!e["riverWidthScale"].empty()) erosion->riverWidthScale = std::stof(e["riverWidthScale"]);
            if (!e["maxRiverWidth"].empty()) erosion->maxRiverWidth = std::stoi(e["maxRiverWidth"]);
            if (!e["riverDepth"].empty()) erosion->riverDepth = std::stof(e["riverDepth"]);
        }
    }
    
    dirty = true;
    TraceLog(LOG_INFO, "Loaded settings from %s", filename.c_str());
    return true;
}
