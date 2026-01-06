/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2026 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pfconfig.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Stockfish {

namespace {

constexpr int IronMaskRanks = 10;
constexpr int IronMaskFiles = 9;

std::vector<std::filesystem::path> config_roots(const std::string& rootDirectory) {
    std::vector<std::filesystem::path> roots;

    if (!rootDirectory.empty())
        roots.emplace_back(rootDirectory);
    else
        roots.emplace_back(std::filesystem::current_path());

    return roots;
}

template<typename Handler>
bool try_parse_config(const std::vector<std::filesystem::path>& roots,
                      const std::string&                        name,
                      Handler                                   handler,
                      std::string&                              summary) {

    for (const auto& root : roots)
    {
        auto path = root / name;
        std::ifstream file(path);
        if (!file)
            continue;

        handler(file);
        summary = path.string();
        return true;
    }

    return false;
}

Bitboard parse_iron_mask(std::istream& in) {
    Bitboard    mask = 0;
    std::string line;

    for (int row = 0; row < IronMaskRanks && std::getline(in, line); ++row)
    {
        for (int col = 0; col < std::min<int>(IronMaskFiles, static_cast<int>(line.size())); ++col)
        {
            if (line[col] != '1')
                continue;

            auto sq =
              make_square(File(FILE_A + col), Rank(RANK_9 - row));  // First line is rank 9 (top).
            mask |= square_bb(sq);
        }
    }

    return mask;
}

}  // namespace

PFVariantConfig load_pf_config(const std::string& rootDirectory) {
    PFVariantConfig config{};

    auto roots = config_roots(rootDirectory);

    try_parse_config(roots,
                     "tieJiang.pfConfig",
                     [&](std::istream& in) {
                         char flag = 0;
                         while (in.get(flag))
                         {
                             if (flag == '0' || flag == '1')
                             {
                                 config.kingTied = flag == '1';
                                 break;
                             }
                         }
                     },
                     config.sourceSummary);

    std::string tieZiSummary;
    try_parse_config(roots,
                     "tieZi.pfConfig",
                     [&](std::istream& in) { config.ironSquares = parse_iron_mask(in); },
                     tieZiSummary);

    if (!config.sourceSummary.empty() && !tieZiSummary.empty())
        config.sourceSummary += " | ";

    config.sourceSummary += tieZiSummary;

    return config;
}

}  // namespace Stockfish
