// Durchsatzmessung der Messkette. Beantwortet die Frage, wieviel Rechenzeit die
// DSP-Kette auf dem Zielsystem braucht.
//
// Bauen und laufen lassen:  make -C firmware bench
#include <chrono>
#include <initializer_list>
#include <cmath>
#include <cstdio>

#include "../../src/meter.h"

int main() {
  constexpr float kFs = 622.0f;
  constexpr float kSeconds = 60.0f;
  const int n = static_cast<int>(kFs * kSeconds);

  for (int nFft : {512, 1024}) {
    vu::Meter m;
    if (!m.init(kFs, vu::Band::k10to100, nFft)) {
      std::printf("init fehlgeschlagen\n");
      return 1;
    }
    const float amp = vu::paFromSpl(150.0f) * std::sqrt(2.0f);
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < n; ++i) {
      const float t = static_cast<float>(i) / kFs;
      m.process(101325.0f +
                amp * std::sin(2.0f * static_cast<float>(M_PI) * 40.0f * t));
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double dt =
        std::chrono::duration<double>(t1 - t0).count();
    std::printf("N=%4d: %.1f s Messzeit in %.1f ms Rechenzeit -> %.0fx Echtzeit, "
                "%.3f us pro Sample\n",
                nFft, kSeconds, dt * 1e3, kSeconds / dt, dt / n * 1e6);
  }
  return 0;
}
