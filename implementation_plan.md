# World Generation System Specification: "Geological Feedback"

## Overview
This system reverses the traditional "terrain-first" approach. Instead of painting biomes onto a heightmap, we generate **Fundamental Potentials** (Magmatic, Hydrological, Sulfite, Crystalline, Biological) first. These potentials, combined with Climate data, drive the **Biome Selection**, which in turn dictates the **Terrain Generation**.

Crucially, the system includes a **Feedback Loop**: the generated terrain modifies the original potentials, ensuring that resources like ores are found in logically sound locations (e.g., sulfites exposed in canyon walls, crystals in impact craters).

---

## 1. Generation Pipeline

### Step A: Fundamental Potentials (Noise Generation)
Generate 2D floating-point maps (0.0 - 1.0) for each potential using layered noise (Simplex/Perlin).

| Potential | Noise Characteristics | Represents |
| :--- | :--- | :--- |
| **Magmatic** | Low freq, high amplitude | Volcanic activity, tectonic stress, heat sources. |
| **Hydrological** | Medium freq, warped domain | Water table depth, aquifer pressure, erosion potential. |
| **Sulfite** | High freq, cellular (Worley) | Sulfur deposits, acidic soil, chemical reactivity. |
| **Crystalline** | High freq, rigid/angular | Mineral density, gem formation, hard bedrock. |
| **Biological** | Mixed freq (fractal) | Biomass density, growth speed, organic matter. |

**Climate Layer:**
*   **Temperature:** Global gradient + noise variation.
*   **Humidity:** Linked to Hydrological potential + wind offset.

### Step B: Biome Determination
Biomes are strictly defined by `Temperature`, `Humidity`, and dominant `Potential`.

*   **Logic:**
    1.  Calculate base biome from `Temp` / `Humidity` (e.g., Desert, Tundra, Rainforest).
    2.  **Override:** If a specific Potential exceeds a threshold (e.g., `Magmatic > 0.85`), force a "Geological Biome" regardless of climate.

**Example Biome Table:**
| Temperature | Humidity | Dominant Potential | **Resulting Biome** |
| :--- | :--- | :--- | :--- |
| Hot | Low | *None* | **Arid Desert** |
| Hot | Low | **Magmatic** | **Volcanic Ash Wastes** |
| Cold | High | **Crystalline** | **Glacial Spire Field** |
| Moderate | Moderate | **Hydrological** | **Karst Wetlands** |
| Hot | Moderate | **Sulfite** | **Acidic Fumarole Plains** |

### Step C: Biome-Driven Terrain Generation
Each biome acts as a "kernel" that generates height data differently.

*   **Volcanic Ash Wastes:**
    *   *Base:* Smooth, rolling hills (billowy noise).
    *   *Feature:* Large cone structures (SDF addition) at local Magmatic maxima.
    *   *Texture:* Rough, porous surface noise.
*   **Karst Wetlands:**
    *   *Base:* Flat plane at sea level.
    *   *Feature:* Subtractive "sinkholes" (inverted cones) where Hydrological potential is high.
    *   *Texture:* Smooth mud noise.
*   **Crystalline Spire Field:**
    *   *Base:* jagged ridged-multifractal noise.
    *   *Feature:* Vertical hexagonal prisms (Voronoi extrusion) added to heightmap.

---

## 2. Geological Feedback Loop (The "Secret Sauce")

Once the initial terrain `H_init` is generated, we analyze it to **modify** the resource potentials. This makes exploration intuitive: "I need sulfites, so I should look for eroded cliffs."

### Analysis Pass
Calculate three maps from `H_init`:
1.  **Slope Map:** Steepness of terrain.
2.  **Curvature Map:** Concavity/Convexity (peaks vs. valleys).
3.  **Flow Map:** Simple downhill accumulation (pseudo-hydrology).

### Modification Rules
Apply these modifiers to the original Potential maps to create `Final_Potentials`:

1.  **Erosion Exposure (Sulfite/Crystalline):**
    *   *Logic:* Erosion exposes buried strata.
    *   *Rule:* `Sulfite_Final = Sulfite_Base + (Slope_Map * 0.5)`
    *   *Effect:* Cliffs and steep canyon walls become hotbeds for sulfite ores.

2.  **Sediment Deposition (Biological/Hydrological):**
    *   *Logic:* Water and nutrients settle in low, flat areas.
    *   *Rule:* `Bio_Final = Bio_Base + (Flow_Map * 0.3) - (Slope_Map * 0.2)`
    *   *Effect:* Valleys are lush; peaks are barren.

3.  **Impact Concentration (Crystalline):**
    *   *Logic:* Meteor impacts or geysers create specific mineral concentrations.
    *   *Rule:* If `Curvature` is highly concave (crater bottom), boost `Crystalline`.

---

## 3. Optional: Hydraulic Erosion Simulation
*Enable this for high-fidelity "Factory Scouting" gameplay.*

This step runs *between* Step C and the Feedback Loop.

**Algorithm:** Particle-Based Hydraulic Erosion (Simplified) [web:55][web:62]

```

def simulate_erosion(terrain, potential_map, iterations=50000):
for i in range(iterations):
\# 1. Spawn Drop
\# Spawn particle at random (x,y)
\# Or bias spawn towards high Hydrological Potential areas
x, y = random_spawn(potential_map.hydrological)

        water = 1.0
        sediment = 0.0
        
        while water > 0.01:
            # 2. Calculate Gradient
            gradient = calculate_gradient(terrain, x, y)
            
            # 3. Move
            new_x = x + gradient.x
            new_y = y + gradient.y
            
            # 4. Erode/Deposit
            height_diff = terrain[x,y] - terrain[new_x, new_y]
            capacity = max(height_diff, 0.01) * water * SPEED
            
            if sediment > capacity:
                # Deposit
                amount = (sediment - capacity) * DEPOSITION_RATE
                terrain[x,y] += amount
                sediment -= amount
            else:
                # Erode
                amount = min((capacity - sediment) * EROSION_RATE, -height_diff)
                terrain[x,y] -= amount
                sediment += amount
                
                # CRITICAL: Update Feedback Map
                # Mark this coordinate as "Eroded" to boost Sulfite/Crystalline potential later
                erosion_intensity_map[x,y] += amount
            
            # 5. Evaporate
            water *= (1.0 - EVAPORATION_RATE)
            x, y = new_x, new_y
    ```

**Integration:**
*   The `erosion_intensity_map` generated here feeds directly into the Feedback Loop.
*   High erosion intensity = Massive boost to `Crystalline` and `Sulfite` visibility (ore veins exposed).

---

## 4. Data Structures & Interfaces

**PotentialMap Struct:**
```

struct PotentialMap {
float magmatic;
float hydrological;
float sulfite;
float crystalline;
float biological;
float temperature;
float humidity;
};

```

**Biome Interface:**
```

interface Biome {
// Returns height offset at (x,y) based on biome-specific logic
float GetHeight(int x, int y, PotentialMap potentials);

    // Returns color/material data
    Material GetSurfaceMaterial(int x, int y, float slope, float curvature);
    }

```

## 5. Scouting Gameplay Implications
*   **Magmatic:** Players scout for volcanic cones to find heat sources for power.
*   **Hydrological:** Players follow river beds upstream to find "source springs" (high potential) for water pumps.
*   **Sulfite:** Players look for steep cliffs or yellow-stained eroded canyons for chemical production.
*   **Crystalline:** Players look for "unnatural" jagged spires or impact craters for advanced optical components.
```
This will also be used to modify some of the potentials, basically treating hydrological and humidity as rough simplifications, that are then refined with this step, we may also use this to modify the water behaviour to actually form lakes, ponds and rivers instead of the simple water level we currently use, maybe rivers? not sure