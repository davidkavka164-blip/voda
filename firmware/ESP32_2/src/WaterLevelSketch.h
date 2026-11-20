#ifndef HWC_WATER_LEVEL_SKETCH_H
#define HWC_WATER_LEVEL_SKETCH_H

#include <Arduino.h>
#include <cmath>

namespace HWC {

static const char WATER_LEVEL_SKETCH_CSS[] =
    "<style>"
    ".level-sketches{display:flex;flex-wrap:wrap;gap:14px;margin:12px 0 20px}"
    ".level-sketch{flex:1 1 215px;max-width:320px;min-width:0;margin:0;padding:14px;"
    "border:1px solid #ccdbe4;border-radius:12px;background:#f7fafc}"
    ".level-sketch figcaption{font-weight:bold;color:#153447}"
    ".level-display{display:flex;align-items:stretch;justify-content:center;gap:8px;margin:12px 0}"
    ".level-vessel{box-sizing:border-box;position:relative;overflow:hidden;width:155px;max-width:68%;"
    "height:108px;border:2px solid #426478;border-radius:10px;background:#f3f7fa}"
    ".level-vessel.well{width:66px;border-radius:28px 28px 14px 14px}"
    ".level-vessel.offline{border-style:dashed;background:#eaf0f3}"
    ".level-water{position:absolute;bottom:0;left:0;right:0;background:#237eb4;"
    "background:linear-gradient(#70bddb,#237eb4);border-top:2px solid #155d89;box-sizing:border-box}"
    ".level-scale{display:flex;flex-direction:column;justify-content:space-between;min-width:48px;"
    "font-size:12px;color:#344f60}"
    ".level-value{display:block;color:#153447;font-size:1.12em}"
    ".level-note{display:block;margin-top:3px;color:#485d69;font-size:.84em}"
    "</style>";

inline void appendWaterLevelSketch(String& html, const char* label, bool isWell,
                                  bool readingValid, float levelCm, float maxCm) {
    const bool measured = readingValid && std::isfinite(levelCm);
    const bool scaleValid = std::isfinite(maxCm) && maxCm > 0.0f;
    const bool drawWater = measured && scaleValid && levelCm >= 0.0f && levelCm <= maxCm;

    html += "<figure class='level-sketch'><figcaption>";
    html += label; // Static application label.
    html += "</figcaption><div class='level-display' aria-hidden='true'><div class='level-vessel";
    if (isWell) html += " well";
    if (!drawWater) html += " offline";
    html += "'>";

    if (drawWater && levelCm > 0.0f) {
        html += "<div class='level-water' style='height:";
        html += String(100.0f * (levelCm / maxCm), 2);
        html += "%'></div>";
    }
    html += "</div>";
    if (scaleValid) {
        html += "<div class='level-scale'><span>";
        html += String(maxCm, 1);
        html += " cm</span><span>0 cm</span></div>";
    }
    html += "</div><strong class='level-value'>";
    if (measured) {
        html += String(levelCm, 1);
        html += " cm";
    } else {
        html += "Hladina nedostupná";
    }
    html += "</strong>";
    if (!measured) html += "<small class='level-note'>Chybí platné aktuální měření.</small>";
    else if (!scaleValid) html += "<small class='level-note'>Rozsah nádrže není nastavený.</small>";
    else if (!drawWater) html += "<small class='level-note'>Hladina je mimo rozsah nákresu.</small>";
    html += "</figure>";
}

} // namespace HWC

#endif
