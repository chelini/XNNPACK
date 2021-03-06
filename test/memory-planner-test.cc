// Copyright 2020 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <xnnpack.h>
#include <xnnpack/memory-planner.h>
#include <xnnpack/subgraph.h>

#include <gtest/gtest.h>

TEST(MemoryPlanner, ValueLiveInfo) {
  EXPECT_EQ(xnn_status_success, xnn_initialize(nullptr /* allocator */));
  // Create simple subgraph where it has 2 nodes and 4 tensors as illustrated below:
  // T0 ----> N0 ----> T2  and T2 ----> N1 ----> T3
  // T1 ----/                  T1 ----/
  struct xnn_subgraph subgraph;
  subgraph.num_values = 4;
  subgraph.num_nodes = 2;
  struct xnn_node nodes[2];
  nodes[0].num_inputs = 2;
  nodes[0].inputs[0] = 0;
  nodes[0].inputs[1] = 1;
  nodes[0].num_outputs = 1;
  nodes[0].outputs[0] = 2;

  nodes[1].num_inputs = 2;
  nodes[1].inputs[0] = 1;
  nodes[1].inputs[1] = 2;
  nodes[1].num_outputs = 1;
  nodes[1].outputs[0] = 3;
  subgraph.nodes = nodes;

  struct xnn_value_allocation_tracker tracker;
  xnn_init_value_allocation_tracker(&tracker, &subgraph);

  EXPECT_EQ(0, tracker.usage[0].first_node);
  EXPECT_EQ(0, tracker.usage[0].last_node);

  EXPECT_EQ(0, tracker.usage[1].first_node);
  EXPECT_EQ(1, tracker.usage[1].last_node);

  EXPECT_EQ(0, tracker.usage[2].first_node);
  EXPECT_EQ(1, tracker.usage[2].last_node);

  EXPECT_EQ(1, tracker.usage[3].first_node);
  EXPECT_EQ(1, tracker.usage[3].last_node);

  xnn_release_value_allocation_tracker(&tracker);
}

TEST(MemoryPlanner, MemoryBlocksCoalescing) {
  EXPECT_EQ(xnn_status_success, xnn_initialize(nullptr /* allocator */));
  struct xnn_subgraph subgraph;
  subgraph.num_nodes = 0;
  subgraph.num_values = 5;
  struct xnn_value_allocation_tracker tracker;
  xnn_init_value_allocation_tracker(&tracker, &subgraph);
  // As this is an empty subgraph, we create the following xnn_value_usage stub.
  tracker.usage[0] = (struct xnn_value_usage){
      .first_node = 1,
      .last_node = 1,
  };
  xnn_add_value_allocation_tracker(&tracker, 0, 56);

  tracker.usage[1] = (struct xnn_value_usage){
      .first_node = 0,
      .last_node = 1,
  };
  xnn_add_value_allocation_tracker(&tracker, 1, 40);

  tracker.usage[2] = (struct xnn_value_usage){
      .first_node = 1,
      .last_node = 1,
  };
  xnn_add_value_allocation_tracker(&tracker, 2, 64);

  tracker.usage[3] = (struct xnn_value_usage){
      .first_node = 0,
      .last_node = 0,
  };
  xnn_add_value_allocation_tracker(&tracker, 3, 152);

  tracker.usage[4] = (struct xnn_value_usage){
      .first_node = 1,
      .last_node = 1,
  };
  xnn_add_value_allocation_tracker(&tracker, 4, 20);

  xnn_plan_value_allocation_tracker(&tracker);

#if XNN_ENABLE_MEMOPT
  EXPECT_EQ(192, tracker.mem_arena_size);
  EXPECT_EQ(64, tracker.usage[0].alloc_offset);
  EXPECT_EQ(152, tracker.usage[1].alloc_offset);
  EXPECT_EQ(0, tracker.usage[2].alloc_offset);
  EXPECT_EQ(0, tracker.usage[3].alloc_offset);
  EXPECT_EQ(120, tracker.usage[4].alloc_offset);
#else
  EXPECT_EQ(332, tracker.mem_arena_size);
  EXPECT_EQ(0, tracker.usage[0].alloc_offset);
  EXPECT_EQ(57, tracker.usage[1].alloc_offset);
  EXPECT_EQ(96, tracker.usage[2].alloc_offset);
  EXPECT_EQ(160, tracker.usage[3].alloc_offset);
  EXPECT_EQ(312, tracker.usage[4].alloc_offset);
#endif

  xnn_release_value_allocation_tracker(&tracker);
}

TEST(MemoryPlanner, GeneralPlanning) {
  EXPECT_EQ(xnn_status_success, xnn_initialize(nullptr /* allocator */));
  struct xnn_subgraph subgraph;
  subgraph.num_nodes = 0;
  subgraph.num_values = 8;
  struct xnn_value_allocation_tracker tracker;
  xnn_init_value_allocation_tracker(&tracker, &subgraph);
  // As this is an empty subgraph, we create the following xnn_value_usage stub.
  tracker.usage[0] = (struct xnn_value_usage){
      .first_node = 0,
      .last_node = 1,
  };
  xnn_add_value_allocation_tracker(&tracker, 0, 32);

  tracker.usage[1] = (struct xnn_value_usage){
      .first_node = 1,
      .last_node = 4,
  };
  xnn_add_value_allocation_tracker(&tracker, 1, 28);

  tracker.usage[2] = (struct xnn_value_usage){
      .first_node = 2,
      .last_node = 5,
  };
  xnn_add_value_allocation_tracker(&tracker, 2, 36);

  tracker.usage[3] = (struct xnn_value_usage){
      .first_node = 3,
      .last_node = 5,
  };
  xnn_add_value_allocation_tracker(&tracker, 3, 16);

  tracker.usage[4] = (struct xnn_value_usage){
      .first_node = 4,
      .last_node = 5,
  };
  xnn_add_value_allocation_tracker(&tracker, 4, 8);

  tracker.usage[5] = (struct xnn_value_usage){
      .first_node = 5,
      .last_node = 7,
  };
  xnn_add_value_allocation_tracker(&tracker, 5, 64);

  tracker.usage[6] = (struct xnn_value_usage){
      .first_node = 6,
      .last_node = 8,
  };
  xnn_add_value_allocation_tracker(&tracker, 6, 10);

  tracker.usage[7] = (struct xnn_value_usage){
      .first_node = 7,
      .last_node = 8,
  };
  xnn_add_value_allocation_tracker(&tracker, 7, 40);

  xnn_plan_value_allocation_tracker(&tracker);

#if XNN_ENABLE_MEMOPT
  EXPECT_EQ(124, tracker.mem_arena_size);
  EXPECT_EQ(0, tracker.usage[0].alloc_offset);
  EXPECT_EQ(32, tracker.usage[1].alloc_offset);
  EXPECT_EQ(64, tracker.usage[2].alloc_offset);
  EXPECT_EQ(100, tracker.usage[3].alloc_offset);
  EXPECT_EQ(116, tracker.usage[4].alloc_offset);
  EXPECT_EQ(0, tracker.usage[5].alloc_offset);
  EXPECT_EQ(104, tracker.usage[6].alloc_offset);
  EXPECT_EQ(64, tracker.usage[7].alloc_offset);
#else
  EXPECT_EQ(234, tracker.mem_arena_size);
  EXPECT_EQ(0, tracker.usage[0].alloc_offset);
  EXPECT_EQ(32, tracker.usage[1].alloc_offset);
  EXPECT_EQ(60, tracker.usage[2].alloc_offset);
  EXPECT_EQ(96, tracker.usage[3].alloc_offset);
  EXPECT_EQ(112, tracker.usage[4].alloc_offset);
  EXPECT_EQ(120, tracker.usage[5].alloc_offset);
  EXPECT_EQ(184, tracker.usage[6].alloc_offset);
  EXPECT_EQ(194, tracker.usage[7].alloc_offset);
#endif

  xnn_release_value_allocation_tracker(&tracker);
}
