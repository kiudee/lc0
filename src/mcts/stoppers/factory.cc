/*
  This file is part of Leela Chess Zero.
  Copyright (C) 2019 The LCZero Authors

  Leela Chess is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Leela Chess is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Leela Chess.  If not, see <http://www.gnu.org/licenses/>.

  Additional permission under GNU GPL version 3 section 7

  If you modify this Program, or any covered work, by linking or
  combining it with NVIDIA Corporation's libraries from the NVIDIA CUDA
  Toolkit and the NVIDIA CUDA Deep Neural Network library (or a
  modified version of those libraries), containing parts covered by the
  terms of the respective license agreement, the licensors of this
  Program grant you additional permission to convey the resulting work.
*/

#include "mcts/stoppers/factory.h"

#include <optional>

#include "factory.h"
#include "mcts/stoppers/alphazero.h"
#include "mcts/stoppers/legacy.h"
#include "mcts/stoppers/smooth.h"
#include "mcts/stoppers/stoppers.h"
#include "utils/exception.h"

namespace lczero {
namespace {

const OptionId kMoveOverheadId{
    "move-overhead", "MoveOverheadMs",
    "Amount of time, in milliseconds, that the engine subtracts from it's "
    "total available time (to compensate for slow connection, interprocess "
    "communication, etc)."};
const OptionId kTimeManagerId{
    "time-manager", "TimeManager",
    "Name and config of a time manager. "
    "Possible names are 'legacy', 'smooth' (default) and 'alphazero'."
    "See https://lc0.org/timemgr for configuration details."};

const OptionId kMPId{"midpoint-move", "midpoint-move",""};
const OptionId kSTId{"steepness", "steepness",""};
const OptionId kITRId{"init-tree-reuse", "init-tree-reuse",""};
const OptionId kMTRId{"max-tree-reuse", "max-tree-reuse",""};
const OptionId kTRURId{"tree-reuse-update-rate", "tree-reuse-update-rate",""};
const OptionId kNURId{"nps-update-rate", "nps-update-rate",""};
const OptionId kITId{"init-timeuse", "init-timeuse",""};
const OptionId kMTId{"min-timeuse", "min-timeuse",""};
const OptionId kTURId{"timeuse-update-rate", "timeuse-update-rate",""};
const OptionId kMMBId{"max-move-budget", "max-move-budget",""};
const OptionId kIPId{"init-piggybank", "init-piggybank",""};
const OptionId kPMPId{"per-move-piggybank", "per-move-piggybank",""};
const OptionId kMPUId{"max-piggybank-use", "max-piggybank-use",""};
const OptionId kMPMId{"max-piggybank-moves", "max-piggybank-moves",""};
}  // namespace


void PopulateTimeManagementOptions(RunType for_what, OptionsParser* options) {
  PopulateCommonStopperOptions(for_what, options);
  if (for_what == RunType::kUci) {
    options->Add<IntOption>(kMoveOverheadId, 0, 100000000) = 200;
    options->Add<FloatOption>(kMPId, 1, 100) = 51.5f;
    options->Add<FloatOption>(kSTId, 1, 10) = 7.0f;
    options->Add<FloatOption>(kITRId, 0, 10) = 0.5f;
    options->Add<FloatOption>(kMTRId, 0, 10) = 0.8f;
    options->Add<FloatOption>(kTRURId, 0, 10) = 3.0f;
    options->Add<FloatOption>(kNURId, 0, 10) = 5.0f;
    options->Add<FloatOption>(kITId, 0, 10) = 0.5f;
    options->Add<FloatOption>(kMTId, 0, 10) = 0.2f;
    options->Add<FloatOption>(kTURId, 0, 10) = 3.0f;
    options->Add<FloatOption>(kMMBId, 0, 10) = 0.3f;
    options->Add<FloatOption>(kIPId, 0, 10) = 0.0f;
    options->Add<FloatOption>(kPMPId, 0, 10) = 0.18f;
    options->Add<FloatOption>(kMPUId, 0, 10) = 0.95f;
    options->Add<FloatOption>(kMPMId, 0, 100) = 27.0f;

  }
}

std::unique_ptr<TimeManager> MakeTimeManager(const OptionsDict& options) {
  const int64_t move_overhead = options.Get<int>(kMoveOverheadId);

  std::ostringstream oss;
  oss << "smooth(";
  oss << "midpoint-move=" << std::setprecision(5) << std::fixed << options.Get<float>(kMPId);
  oss << ",steepness=" << std::setprecision(5) << std::fixed << options.Get<float>(kSTId);
  oss << ",init-tree-reuse=" << std::setprecision(5) << std::fixed << options.Get<float>(kITRId);
  oss << ",max-tree-reuse=" << std::setprecision(5) << std::fixed << options.Get<float>(kMTRId);
  oss << ",tree-reuse-update-rate=" << std::setprecision(5) << std::fixed << options.Get<float>(kTRURId);
  oss << ",nps-update-rate=" << std::setprecision(5) << std::fixed << options.Get<float>(kNURId);
  oss << ",init-timeuse=" << std::setprecision(5) << std::fixed << options.Get<float>(kITId);
  oss << ",min-timeuse=" << std::setprecision(5) << std::fixed << options.Get<float>(kMTId);
  oss << ",timeuse-update-rate=" << std::setprecision(5) << std::fixed << options.Get<float>(kTURId);
  oss << ",max-move-budget=" << std::setprecision(5) << std::fixed << options.Get<float>(kMMBId);
  oss << ",init-piggybank=" << std::setprecision(5) << std::fixed << options.Get<float>(kIPId);
  oss << ",per-move-piggybank=" << std::setprecision(5) << std::fixed << options.Get<float>(kPMPId);
  oss << ",max-piggybank-use=" << std::setprecision(5) << std::fixed << options.Get<float>(kMPUId);
  oss << ",max-piggybank-moves=" << std::setprecision(5) << std::fixed << options.Get<float>(kMPMId);
  oss << ")";

  OptionsDict tm_options;
  tm_options.AddSubdictFromString(oss.str());

  const auto managers = tm_options.ListSubdicts();

  std::unique_ptr<TimeManager> time_manager;
  if (managers.size() != 1) {
    throw Exception("Exactly one time manager should be specified, " +
                    std::to_string(managers.size()) + " specified instead.");
  }

  if (managers[0] == "legacy") {
    time_manager =
        MakeLegacyTimeManager(move_overhead, tm_options.GetSubdict("legacy"));
  } else if (managers[0] == "alphazero") {
    time_manager = MakeAlphazeroTimeManager(move_overhead,
                                            tm_options.GetSubdict("alphazero"));
  } else if (managers[0] == "smooth") {
    time_manager =
        MakeSmoothTimeManager(move_overhead, tm_options.GetSubdict("smooth"));
  }

  if (!time_manager) {
    throw Exception("Unknown time manager: [" + managers[0] + "]");
  }
  tm_options.CheckAllOptionsRead("");

  return MakeCommonTimeManager(std::move(time_manager), options, move_overhead);
}

}  // namespace lczero
