/*--------------------------------------------------------------------*/
/*    Copyright 2002 - 2008, 2010, 2011 National Technology &         */
/*    Engineering Solutions of Sandia, LLC (NTESS). Under the terms   */
/*    of Contract DE-NA0003525 with NTESS, there is a                 */
/*    non-exclusive license for use of this work by or on behalf      */
/*    of the U.S. Government.  Export of this program may require     */
/*    a license from the United States Government.                    */
/*--------------------------------------------------------------------*/




#include <array>
#include <memory>
#include <utility>

#include <gtest/gtest.h>
#include <stk_search/BoundingBox.hpp>
#include <stk_search/IdentProc.hpp>
#include <stk_transfer/GeometricTransfer.hpp>
#include <stk_transfer/ReducedDependencyGeometricTransfer.hpp>
#include <stk_transfer/TransferBase.hpp>
#include <stk_util/parallel/ParallelReduce.hpp>


namespace stk {
namespace transfer {


using EntityKey = unsigned;

class MockMeshA_Common
{
public:
  using EntityProc = stk::search::IdentProc<EntityKey, unsigned>;
  using EntityProcVec = std::vector<EntityProc>;
  using BoundingBox = std::pair<stk::search::Box<double>, EntityProc>;

  MPI_Comm comm() const {return m_comm;}
  MPI_Comm m_comm = MPI_COMM_WORLD;

  void update_values() {}

};

class MockMeshB_Common
{
public:
  using EntityProc = stk::search::IdentProc<EntityKey, unsigned>;
  using EntityProcVec = std::vector<EntityProc>;
  using BoundingBox = std::pair<stk::search::Sphere<double>, EntityProc>;

  //Used for Reduced dependency
  using Point = stk::search::Point<double>;
  using ToPointsContainer = std::vector<Point>;
  using ToPointsDistance = double;
  using ToPointsDistanceContainer = std::vector<ToPointsDistance>;

  MPI_Comm comm() const {return m_comm;}
  MPI_Comm m_comm = MPI_COMM_WORLD;

  void update_values() {}
};

template <typename FROM, typename TO>
class MockInterpolate_Common
{
public:
  using MeshA = FROM;
  using MeshB = TO;
  using EntityKeyA = EntityKey;
  using EntityKeyB = EntityKey;
  using EntityKeySetA = std::set<EntityKeyA>;
  using EntityKeySetB = std::set<EntityKeyB>;
  using EntityProcA = typename MeshA::EntityProc;
  using EntityProcB = typename MeshB::EntityProc;
  using EntityKeyMap = std::multimap<EntityKeyB, EntityKeyA>;
  using EntityProcRelation = std::pair<EntityProcB, EntityProcA>;
  using EntityProcRelationVec = std::vector<EntityProcRelation>;

};

template <class T>
class GeometricTransferExecutor
{
public:
  GeometricTransferExecutor(stk::ParallelMachine global_pm, int color, std::vector<int> mesh_color_ownership) : shared_pm(global_pm)
  {
    stk::ParallelMachine individual_pm;
    MPI_Comm_split(global_pm, color, stk::parallel_machine_rank(global_pm), &individual_pm);

    commOwnsMesh.resize(2);
    commOwnsMesh[0] = color == mesh_color_ownership[0];
    commOwnsMesh[1] = color == mesh_color_ownership[1];
    create_meshes();
  }

  //non mpmd execution
  GeometricTransferExecutor() : shared_pm(MPI_COMM_WORLD)
  {
    commOwnsMesh = {true, true};
    create_meshes();
  }
  void run(stk::search::SearchMethod method, double expansion_factor=1.5)
  {
    create("transfer_name", expansion_factor, method);

    this->transfer->initialize();
    this->transfer->apply();
  }

  std::shared_ptr<typename T::MeshA> meshA;
  std::shared_ptr<typename T::MeshB> meshB;

private:
  std::unique_ptr<stk::transfer::TransferBase> transfer;
  const stk::ParallelMachine shared_pm;
  std::vector<bool> commOwnsMesh;

  void create_meshes() {
    if (commOwnsMesh[0])
    {
      meshA = std::make_shared<typename T::MeshA>();
      meshA->m_comm = shared_pm;
    }
    if (commOwnsMesh[1])
    {
      meshB = std::make_shared<typename T::MeshB>();
      meshB->m_comm = shared_pm;
    }
  }

  void create(
    const std::string & name,
    double expansion_factor,
    stk::search::SearchMethod method)
{
  transfer.reset(new T(meshA, meshB, name, shared_pm, expansion_factor, method));
}
};

class SingleElemMockMeshA : public MockMeshA_Common
{
public:
  int owning_rank() const { return m_owning_rank; }
  void bounding_boxes(std::vector<BoundingBox> & domain_vector) const
  {
    if (stk::parallel_machine_rank(m_comm) == owning_rank())
    {
      stk::search::Box<double> box(0., 0., 0., 1., 1., 1.);
      EntityProc entityProc(3, owning_rank());
      domain_vector.emplace_back(box, entityProc);
    }
  }
  int m_owning_rank = 0;
};


class SinglePointMockMeshB : public MockMeshB_Common
{
public:
  SinglePointMockMeshB ()
{
    m_owning_rank = stk::parallel_machine_size(m_comm) - 1;
}
  int owning_rank() const { return m_owning_rank; }
  Point get_point() const {return Point(0.5, 0.5, 0.5);}
  void bounding_boxes(std::vector<BoundingBox> & range_vector) const
  {
    if (stk::parallel_machine_rank(m_comm) == owning_rank())
    {
      EntityProc entityProc(5, owning_rank());
      range_vector.emplace_back(stk::search::Sphere<double>(get_point(), 1.0e-6), entityProc);
    }
  }

  void pair_communication(const EntityProcVec &to_entity_keys, ToPointsContainer &to_points)
  {
    to_points.push_back(get_point());
  }
  int m_owning_rank;
};

class MockSingleElemToSinglePointInterpolate : public MockInterpolate_Common<SingleElemMockMeshA, SinglePointMockMeshB>
{
public:
  using MeshA = SingleElemMockMeshA;
  using MeshB = SinglePointMockMeshB;
  static unsigned filterSize;
  static int fromCount;
  static void filter_to_nearest(EntityKeyMap & local_range_to_domain, const MeshA & mesha, const MeshB & meshb)
  {
    //no filtering needed since map is one-to-one
  }

  ~MockSingleElemToSinglePointInterpolate()
  {
    filterSize = 0u;
    fromCount = 0;
  }

  //Specific to single point case right now
  void obtain_parametric_coords(typename MeshA::EntityProcVec entities_to_copy_from,
      MeshA &FromElem,
      const typename MeshB::ToPointsContainer & to_points_on_from_mesh,
      typename MeshB::ToPointsDistanceContainer & to_points_distance_on_from_mesh)
  {
    for (unsigned i = 0; i < entities_to_copy_from.size(); ++i)
    {
      to_points_distance_on_from_mesh.push_back(0.0);
    }
  }

  void mask_parametric_coords(const std::vector<int> & filter_mask_from, int from_count)
  {
    EXPECT_EQ(0u, filterSize);
    EXPECT_EQ(0, fromCount);
    filterSize = filter_mask_from.size();
    fromCount = from_count;
  }

  static void apply(const MeshB & meshb, const MeshA & mesha, EntityKeyMap & local_range_to_domain)
  {
    if (stk::parallel_machine_rank(meshb.m_comm) == meshb.owning_rank())
    {
      ASSERT_EQ(1u, local_range_to_domain.size());
      auto matches = local_range_to_domain.equal_range(5);
      ASSERT_EQ(1u, std::distance(matches.first, matches.second));
      EXPECT_EQ(3u, matches.first->second);
    }
    else
    {
      ASSERT_EQ(0u, local_range_to_domain.size());
    }
  }

  void
  apply(MeshB * ToPoints,
      MeshA * FromElem,
      const typename MeshB::EntityProcVec & to_entity_keys_masked,
      const typename MeshA::EntityProcVec & from_entity_keys_masked,
      const ReducedDependencyCommData & comm_data)
  {
    if (ToPoints && stk::parallel_machine_rank(ToPoints->m_comm) == ToPoints->owning_rank())
    {
      EXPECT_EQ(1, comm_data.numToMeshCommunications);
      ASSERT_EQ(1u, to_entity_keys_masked.size());
      EXPECT_EQ(5u, to_entity_keys_masked[0].id());
    }
    else
    {
      EXPECT_EQ(0, comm_data.numToMeshCommunications);
      EXPECT_EQ(0u, to_entity_keys_masked.size());
    }

    if (FromElem && stk::parallel_machine_rank(FromElem->m_comm) == FromElem->owning_rank())
    {
      EXPECT_EQ(1, comm_data.numFromMeshCommunications);
      ASSERT_EQ(1u, from_entity_keys_masked.size());
      EXPECT_EQ(3u, from_entity_keys_masked[0].id());
    }
    else
    {
      EXPECT_EQ(0, comm_data.numFromMeshCommunications);
      EXPECT_EQ(0u, from_entity_keys_masked.size());
    }
  }

};

unsigned MockSingleElemToSinglePointInterpolate::filterSize = 0u;
int MockSingleElemToSinglePointInterpolate::fromCount = 0;

class GeometricTransferTest :
public ::testing::TestWithParam<stk::search::SearchMethod>
{
};

using ReducedDependencyGeometricTransferTest = GeometricTransferTest;


void check_single_elem_to_point_parametric_mask(stk::ParallelMachine transfer_pm, const SingleElemMockMeshA * meshA)
{
  if (meshA && stk::parallel_machine_rank(transfer_pm) == meshA->owning_rank())
  {
    EXPECT_EQ(1u, MockSingleElemToSinglePointInterpolate::filterSize);
    EXPECT_EQ(1, MockSingleElemToSinglePointInterpolate::fromCount);
  }
  else
  {
    EXPECT_EQ(0u, MockSingleElemToSinglePointInterpolate::filterSize);
    EXPECT_EQ(0, MockSingleElemToSinglePointInterpolate::fromCount);
  }
}

TEST_P(GeometricTransferTest, SingleElemToPoint)
{
  GeometricTransferExecutor<stk::transfer::GeometricTransfer<MockSingleElemToSinglePointInterpolate>> test;
  test.run(GetParam());
}

TEST_P(ReducedDependencyGeometricTransferTest, SingleElemToPoint)
{
  GeometricTransferExecutor<stk::transfer::ReducedDependencyGeometricTransfer<MockSingleElemToSinglePointInterpolate>> test;
  test.run(GetParam());
}

TEST_P(ReducedDependencyGeometricTransferTest, MpmdSingleElemToPoint)
{
  if (2 != stk::parallel_machine_size(MPI_COMM_WORLD)) return;
  using TestType = GeometricTransferExecutor<stk::transfer::ReducedDependencyGeometricTransfer<MockSingleElemToSinglePointInterpolate>>;
  const int color = stk::parallel_machine_rank(MPI_COMM_WORLD) % 2;
  TestType test(MPI_COMM_WORLD, color, {0,1});
  test.run(GetParam());
  check_single_elem_to_point_parametric_mask(MPI_COMM_WORLD, test.meshA.get());
}

TEST_P(ReducedDependencyGeometricTransferTest, MpmdSingleElemToPointInvertedOwnership)
{
  if (2 != stk::parallel_machine_size(MPI_COMM_WORLD)) return;
  using TestType = GeometricTransferExecutor<stk::transfer::ReducedDependencyGeometricTransfer<MockSingleElemToSinglePointInterpolate>>;
  const int color = stk::parallel_machine_rank(MPI_COMM_WORLD) % 2;
  TestType test(MPI_COMM_WORLD, color, {1,0});
  if (test.meshA)
    test.meshA->m_owning_rank = 1;
  if (test.meshB)
    test.meshB->m_owning_rank = 0;
  test.run(GetParam());
  check_single_elem_to_point_parametric_mask(MPI_COMM_WORLD, test.meshA.get());
}

TEST_P(ReducedDependencyGeometricTransferTest, MpmdSingleElemToPointInSubCommunicator)
{
  stk::ParallelMachine global_pm = MPI_COMM_WORLD;
  if (3 != stk::parallel_machine_size(global_pm)) return;

  //split such that rank 0 has color 0 and ranks 1 and 2 have color 1
  //such that global comm rank numbers don't agree with transfered shared rank numbers
  const int external_color = stk::parallel_machine_rank(MPI_COMM_WORLD) == 0 ? 0 : 1;
  stk::ParallelMachine transfer_shared_pm;
  MPI_Comm_split(global_pm, external_color, stk::parallel_machine_rank(global_pm), &transfer_shared_pm);
  if (external_color == 0)
  {
    return; //rank 0 doesn't participate
  }

  using TestType = GeometricTransferExecutor<stk::transfer::ReducedDependencyGeometricTransfer<MockSingleElemToSinglePointInterpolate>>;
  const int color = stk::parallel_machine_rank(transfer_shared_pm) % 2;
  TestType test(transfer_shared_pm, color, {0,1});
  if (test.meshA)
    test.meshA->m_owning_rank = 0;
  if (test.meshB)
    test.meshB->m_owning_rank = 1;
  test.run(GetParam());
  check_single_elem_to_point_parametric_mask(transfer_shared_pm, test.meshA.get());
}


class ThreeElemMockMeshA : public MockMeshA_Common
{
public:
  EntityKey elemToFilter;
  EntityKey elemToUse;
  static std::array<int, 3> owning_ranks() { return {0, 1, 0}; }
  void bounding_boxes(std::vector<BoundingBox> & domain_vector) const
  {
    if (stk::parallel_machine_rank(MPI_COMM_WORLD) == owning_ranks()[0])
    {
      stk::search::Box<double> box(0., 0., 0., 1., 1., 1.);
      EntityProc entityProc(0, owning_ranks()[0]);
      domain_vector.emplace_back(box, entityProc);
    }
    if (stk::parallel_machine_rank(MPI_COMM_WORLD) == owning_ranks()[1])
    {
      stk::search::Box<double> box(0.4, 0.4, 0.4, 1.5, 1., 1.);
      EntityProc entityProc(3, owning_ranks()[1]);
      domain_vector.emplace_back(box, entityProc);
    }
    if (stk::parallel_machine_rank(MPI_COMM_WORLD) == owning_ranks()[2])
    {
      stk::search::Box<double> box(1.9, 1.9, 1.9, 2., 2., 2.);
      EntityProc entityProc(5, owning_ranks()[2]);
      domain_vector.emplace_back(box, entityProc);
    }
  }
};


class TwoPointMockMeshB : public MockMeshB_Common
{
public:
  //Both points are going to lie in MeshA entities 0 and 3 and
  //cross processor filtering will be needed
  TwoPointMockMeshB() : pointA(0.5, 0.5, 0.5), pointB(0.6,0.6,0.6) {}

  static std::array<int, 2> owning_ranks() { return {1, 2}; }
  std::array<Point, 2> get_points() const
  {
    return {pointA, pointB};
  }
  void bounding_boxes(std::vector<BoundingBox> & range_vector) const
  {
    for (unsigned i = 0; i < owning_ranks().size(); ++i)
    {
      if (stk::parallel_machine_rank(MPI_COMM_WORLD) == owning_ranks()[i])
      {
        EntityProc entityProc(i, owning_ranks()[i]);
        range_vector.emplace_back(stk::search::Sphere<double>(get_points()[i], 1.0e-6), entityProc);
      }
    }
  }

  void pair_communication(const EntityProcVec &to_entity_keys, ToPointsContainer &to_points)
  {
    for (auto && entityProc : to_entity_keys)
    {
      to_points.push_back(get_points()[entityProc.id()]);
    }
  }
  Point pointA;
  Point pointB;
};

class MockThreeElemToTwoPointsInerpolate : public MockInterpolate_Common<ThreeElemMockMeshA, TwoPointMockMeshB>
{
public:
  using MeshA = ThreeElemMockMeshA;
  using MeshB = TwoPointMockMeshB;
  static void filter_to_nearest(EntityKeyMap & local_range_to_domain, const MeshA & mesha, const MeshB & meshb)
  {
    auto results = local_range_to_domain.equal_range(0);
    for (auto i = results.first; i != results.second; ++i)
    {
      if (i->second == mesha.elemToFilter)
      {
        local_range_to_domain.erase(i);
      }
    }
    results = local_range_to_domain.equal_range(1);
    for (auto i = results.first; i != results.second; ++i)
    {
      if (i->second == mesha.elemToFilter)
      {
        local_range_to_domain.erase(i);
      }
    }
  }

  //Specific to single point case right now
  void obtain_parametric_coords(typename MeshA::EntityProcVec entities_to_copy_from,
      MeshA &FromElem,
      const typename MeshB::ToPointsContainer & to_points_on_from_mesh,
      typename MeshB::ToPointsDistanceContainer & to_points_distance_on_from_mesh)
  {
    for (unsigned i = 0; i < entities_to_copy_from.size(); ++i)
    {
      if (entities_to_copy_from[i].id() == FromElem.elemToUse)
        to_points_distance_on_from_mesh[i] = 0.0;
      else //will force deletion of elemToFIlter
        to_points_distance_on_from_mesh[i] = 1.0;

    }
  }

  void mask_parametric_coords(const std::vector<int> & filter_mask_from, int from_count)
  {
  }

  static void apply(const MeshB & meshb, const MeshA & mesha, EntityKeyMap & local_range_to_domain)
  {
    if (stk::parallel_machine_rank(MPI_COMM_WORLD) == 0)
    {
      ASSERT_EQ(0u, local_range_to_domain.size());
    }
    if (stk::parallel_machine_rank(MPI_COMM_WORLD) == 1)
    {
      ASSERT_EQ(1u, local_range_to_domain.size());
      auto matches = local_range_to_domain.equal_range(0);
      ASSERT_EQ(1u, std::distance(matches.first, matches.second));
      EXPECT_EQ(mesha.elemToUse, matches.first->second);
    }
    if (stk::parallel_machine_rank(MPI_COMM_WORLD) == 2)
    {
      ASSERT_EQ(1u, local_range_to_domain.size());
      auto matches = local_range_to_domain.equal_range(1);
      ASSERT_EQ(1u, std::distance(matches.first, matches.second));
      EXPECT_EQ(mesha.elemToUse, matches.first->second);
    }
  }

  void
  apply(MeshB * ToPoints,
      MeshA * FromElem,
      const typename MeshB::EntityProcVec & to_entity_keys_masked,
      const typename MeshA::EntityProcVec & from_entity_keys_masked,
      const ReducedDependencyCommData & comm_data)
  {
    if (stk::parallel_machine_rank(MPI_COMM_WORLD) == 0)
    {
      ASSERT_EQ(0u, to_entity_keys_masked.size());
      if( FromElem->elemToUse == 0)
      {
        ASSERT_EQ(2u, from_entity_keys_masked.size());
        EXPECT_EQ(0u, from_entity_keys_masked[0].id());
        EXPECT_EQ(1u, from_entity_keys_masked[0].proc());
        EXPECT_EQ(0u, from_entity_keys_masked[1].id());
        EXPECT_EQ(2u, from_entity_keys_masked[1].proc());
      }else {
        EXPECT_EQ(3u, FromElem->elemToUse);
        EXPECT_EQ(0u, from_entity_keys_masked.size());
      }
    }
    if (stk::parallel_machine_rank(MPI_COMM_WORLD) == 1)
    {
      ASSERT_EQ(1u, to_entity_keys_masked.size());
      if( FromElem->elemToUse == 0)
      {
        ASSERT_EQ(0u, from_entity_keys_masked.size());
        EXPECT_EQ(0u, to_entity_keys_masked[0].proc());
      }else {
        EXPECT_EQ(3u, FromElem->elemToUse);
        ASSERT_EQ(2u, from_entity_keys_masked.size());
        EXPECT_EQ(3u, from_entity_keys_masked[0].id());
        EXPECT_EQ(1u, from_entity_keys_masked[0].proc());
        EXPECT_EQ(3u, from_entity_keys_masked[1].id());
        EXPECT_EQ(2u, from_entity_keys_masked[1].proc());
        EXPECT_EQ(1u, to_entity_keys_masked[0].proc());
      }
      EXPECT_EQ(0u, to_entity_keys_masked[0].id());
    }
    if (stk::parallel_machine_rank(MPI_COMM_WORLD) == 2)
    {
      ASSERT_EQ(1u, to_entity_keys_masked.size());
      EXPECT_EQ(1u, to_entity_keys_masked[0].id());

      if( FromElem->elemToUse == 0)
        EXPECT_EQ(0u, to_entity_keys_masked[0].proc());
      else
        EXPECT_EQ(1u, to_entity_keys_masked[0].proc());

      ASSERT_EQ(0u, from_entity_keys_masked.size());
    }
  }

};

TEST_P(GeometricTransferTest, ThreeElemToTwoPoint)
{
  if (stk::parallel_machine_size(MPI_COMM_WORLD) < 3) return;
  GeometricTransferExecutor<stk::transfer::GeometricTransfer<MockThreeElemToTwoPointsInerpolate>> test;
  test.meshA->elemToUse = 0;
  test.meshA->elemToFilter = 3;
  test.run(GetParam());
}

TEST_P(ReducedDependencyGeometricTransferTest, ThreeElemToTwoPoint)
{
  if (stk::parallel_machine_size(MPI_COMM_WORLD) < 3) return;
  GeometricTransferExecutor<stk::transfer::ReducedDependencyGeometricTransfer<MockThreeElemToTwoPointsInerpolate>> test;
  test.meshA->elemToUse = 0;
  test.meshA->elemToFilter = 3;
  test.run(GetParam());
}

TEST_P(GeometricTransferTest, ThreeElemToTwoPointChangeDistance)
{
  if (stk::parallel_machine_size(MPI_COMM_WORLD) < 3) return;
  GeometricTransferExecutor<stk::transfer::GeometricTransfer<MockThreeElemToTwoPointsInerpolate>> test;
  test.meshA->elemToUse = 3;
  test.meshA->elemToFilter = 0;
  test.run(GetParam());
}

TEST_P(ReducedDependencyGeometricTransferTest, ThreeElemToTwoPointChangeDistance)
{
  if (stk::parallel_machine_size(MPI_COMM_WORLD) < 3) return;
  GeometricTransferExecutor<stk::transfer::ReducedDependencyGeometricTransfer<MockThreeElemToTwoPointsInerpolate>> test;
  test.meshA->elemToUse = 3;
  test.meshA->elemToFilter = 0;
  test.run(GetParam());
}

TEST_P(GeometricTransferTest, ThreeElemToTwoPointRequireExpansion)
{
  if (stk::parallel_machine_size(MPI_COMM_WORLD) < 3) return;
  GeometricTransferExecutor<stk::transfer::GeometricTransfer<MockThreeElemToTwoPointsInerpolate>> test;
  test.meshA->elemToUse = 0;
  test.meshA->elemToFilter = 3;
  test.meshB->pointA = stk::search::Point<double>(0.5, 1.01, 0.5);
  test.run(GetParam(), 1.3);
}

TEST_P(ReducedDependencyGeometricTransferTest, ThreeElemToTwoPointRequireExpansion)
{
  if (stk::parallel_machine_size(MPI_COMM_WORLD) < 3) return;
  GeometricTransferExecutor<stk::transfer::ReducedDependencyGeometricTransfer<MockThreeElemToTwoPointsInerpolate>> test;
  test.meshA->elemToUse = 0;
  test.meshA->elemToFilter = 3;
  test.meshB->pointA = stk::search::Point<double>(0.5, 1.01, 0.5);
  test.run(GetParam(), 1.3);
}

TEST_P(GeometricTransferTest, ThreeElemToTwoPointRequireExpansionChangedDistance)
{
  if (stk::parallel_machine_size(MPI_COMM_WORLD) < 3) return;
  GeometricTransferExecutor<stk::transfer::GeometricTransfer<MockThreeElemToTwoPointsInerpolate>> test;
  test.meshA->elemToUse = 3;
  test.meshA->elemToFilter = 0;
  test.meshB->pointA = stk::search::Point<double>(0.5, 1.01, 0.5);
  test.run(GetParam(), 1.3);
}

TEST_P(ReducedDependencyGeometricTransferTest, ThreeElemToTwoPointRequireExpansionChangedDistance)
{
  if (stk::parallel_machine_size(MPI_COMM_WORLD) < 3) return;
  GeometricTransferExecutor<stk::transfer::ReducedDependencyGeometricTransfer<MockThreeElemToTwoPointsInerpolate>> test;
  test.meshA->elemToUse = 3;
  test.meshA->elemToFilter = 0;
  test.meshB->pointA = stk::search::Point<double>(0.5, 1.01, 0.5);
  test.run(GetParam(), 1.3);
}

//Note, these test segfault with the stk::search::SearchMethod::MORTON_LINEARIZED_BVH
//I'm not sure who uses that search method of it is supposed to be production ready
INSTANTIATE_TEST_CASE_P(GeometricTransferTest, GeometricTransferTest,
    ::testing::Values(stk::search::SearchMethod::BOOST_RTREE,
        stk::search::SearchMethod::KDTREE));

INSTANTIATE_TEST_CASE_P(ReducedDependencyGeometricTransferTest, ReducedDependencyGeometricTransferTest,
    ::testing::Values(stk::search::SearchMethod::BOOST_RTREE,
        stk::search::SearchMethod::KDTREE));



}
}
