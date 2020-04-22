/*
 * Copyright (C) 2020 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <cstdio>

#include "PlanningInspector.hpp"

namespace rmf_planning_visualizer
{

std::shared_ptr<PlanningInspector> PlanningInspector::make(
    const Planner& planner)
{
  using Planner = rmf_traffic::agv::Planner;
  std::unique_ptr<Planner::Debug> debugger(new Planner::Debug(planner));

  std::shared_ptr<PlanningInspector> planning_inspector(
      new PlanningInspector(std::move(debugger)));
  
  return planning_inspector;
}

//==============================================================================

PlanningInspector::PlanningInspector(std::unique_ptr<Planner::Debug> debugger)
: _debugger(std::move(debugger))
{}

//==============================================================================

PlanningInspector::~PlanningInspector()
{}

//==============================================================================

bool PlanningInspector::begin(
    const std::vector<Plan::Start>& starts,
    Plan::Goal goal,
    Planner::Options options)
{
  auto progress = _debugger->begin(
      starts,
      std::move(goal),
      std::move(options));
  
  if (!progress)
    return false;

  using Progress = Planner::Debug::Progress;
  _progress.reset(new Progress(std::move(progress)));

  const PlanningState zeroth_step {
    0,
    rmf_utils::nullopt,
    _progress->expanded_nodes(),
    _progress->terminal_nodes()
  };

  _planning_states.clear();
  _planning_states.emplace_back(new PlanningState(std::move(zeroth_step)));
  return true;
}

//==============================================================================

void PlanningInspector::step()
{
  if (!_progress)
    return;
  
  const auto new_plan = _progress->step();
  const PlanningState new_step {
    _planning_states.size(),
    std::move(new_plan),
    _progress->expanded_nodes(),
    _progress->terminal_nodes()
  };

  _planning_states.emplace_back(new PlanningState(std::move(new_step)));
}

//==============================================================================

auto PlanningInspector::plan() const -> rmf_utils::optional<Plan>
{
  if (_planning_states.empty())
    return rmf_utils::nullopt;
  return get_state()->plan;
}

//==============================================================================

bool PlanningInspector::plan_completed() const
{
  if (plan())
    return true;
  return false;
}

//==============================================================================

std::size_t PlanningInspector::step_num() const
{
  return _planning_states.size();
}

//==============================================================================

auto PlanningInspector::get_state() const -> ConstPlanningStatePtr
{
  return _planning_states.back();
}

//==============================================================================

auto PlanningInspector::get_state(std::size_t step_index) const -> ConstPlanningStatePtr
{
  if (step_index > step_num())
    return nullptr;
  return _planning_states[step_index];
}

//==============================================================================

void PlanningInspector::PlanningState::print_plan(
    const Planner::Debug::ConstNodePtr& node) const
{
  std::string expanded_nodes_string = 
      "(" + std::to_string(node->waypoint.value()) + ", " +
      std::to_string(node->orientation) + ")";
  
  Planner::Debug::ConstNodePtr np = node->parent;
  while (np && np->waypoint.has_value())
  {
    expanded_nodes_string =
        "(" + std::to_string(np->waypoint.value()) + ", " +
        std::to_string(np->orientation) + ") -> " + expanded_nodes_string;
    np = np->parent;
  }

  expanded_nodes_string = "(begin) -> " + expanded_nodes_string;
  printf("    %s\n", expanded_nodes_string.c_str());
}

//==============================================================================

void PlanningInspector::PlanningState::print() const
{
  printf("STEP %zu:\n", step_index);
  for (const auto& n : expanded_nodes)
    print_plan(n);

  printf("\n\n");
  if (plan)
    printf("    PLANNING DONE!\n\n");
}

//==============================================================================

} // namespace rmf_planning_visualizer
