/* Copyright Hewlett Packard Enterprise Development LP. */

/**
 * @file test_ltf_moyos.cc
 * @brief Contains test cases for ltf moyos.
 */

#include "u_module.h"
#define __MODULE__ MODULE_test

#include <gmock/gmock.h>

#include "atm_storage_ary.h"
#include "atm_sync_op.h"
#include "common_gtest.h"
#include "obj_base_node.h"
#include "raid_node.h"
#include "raid_sub_ctrl_vector.h"
#include "test_wb_common.h"
#include "test_wb_object.h"
#include "test_wb_txn_common.h"
#include "wb_copy_forward.h"
#include "wb_ctrl_vector.h"
#include "wb_dsp_impl.h"
#include "wb_journal.h"
#include "wb_moyos.h"
#include "wb_node_impl.h"
#include "wb_object_pool_impl.h"
#include "wb_sub_ctrl_vector_impl.h"
#include "wb_superblock.h"
#include "wb_types.h"
#include "WbConfigData.h"
#include "ltf_moyo.pb.h"
#include "ltf_moyo.grpc.pb.h"


using namespace ltf;
using ::testing::HasSubstr;

pf_option_noarg_t opt_legacy_copy_forward(
    "legacy_copy_forward", "Enable legacy copy forward config for tests");

/**
 * @brief Nvo payload size used to create segment sized transaction
 */
const size_t PAYLOAD_segment_size =
    512 * 1024 - 2 * static_cast<uint32_t>(FORMAT_sector_size);

/*
 * Overridden method.
 */
const uint32_t
ObjectPoolImpl::get_recovery_batch_size() const
{
    return 5;
}

/**
 * @brief Override head/tail collision
 *        This allows overwrite of tail so cannot be used with recovery
 */
bool g_override_collision = true;

bool
DspImpl::_handle_journal_full(uint32_t new_real_head_segment_id)
{
    if (g_override_collision) {
        g_test_handle_journal_full(this,
                                   new_real_head_segment_id,
                                   &_real_tail_segment_id);

        return false;
    } else {
        return true;
    }
}

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "ltf_moyo.grpc.pb.h"

class MockMoyoImpl : public MoyoImpl {
public:
    MockMoyoImpl() : MoyoImpl() {}

    MOCK_METHOD(Status, ltfTraceRecordEnable,
                (const LtfTraceRecordEnableRequest* request,
                 LtfTraceRecordEnableResponse* response),
                (override));
};

TEST(MockMoyoImplTest, LtfTraceRecordEnable) {
    MockMoyoImpl mock_moyo_impl;

    LtfTraceRecordEnableRequest request;
    request.set_file_path("/tmp/traces/capture_001.ltf");
    request.set_sampling_rate(0.1);
    request.set_duration_seconds(300);

    LtfTraceRecordEnableResponse response;
    Status status;

    EXPECT_CALL(mock_moyo_impl, ltfTraceRecordEnable(request, response))
        .WillOnce(testing::Return(status));

    // Call the method and check the status
    status = mock_moyo_impl.ltfTraceRecordEnable(&request, &response);

    // Add assertions to check the status and response
    EXPECT_TRUE(status.ok());
    // Add more assertions as per your acceptance criteria
}

/**
 * @class TestLtfMoyos
 * @brief Fixture class for testing Ltf moyos
 *
 */
class TestLtfMoyos : public GTestObjPfCtrlVecFixture, public RestartStack {
public: /* Types */
    typedef std::set<NvObjectId> NvoSet;
    typedef std::vector<NvObjectBase*> NvObjectBaseVector;

public: /* Constants */
    /**
     * @brief Vector representing the superblock fields.
     */
    const std::vector<CorruptRequestType::Field> superblock_fields{
        CorruptRequestType::MAGIC,
        CorruptRequestType::VERSION,
        CorruptRequestType::GENERATION_ID,
        CorruptRequestType::SEGMENT_COUNT,
        CorruptRequestType::REAL_HEAD_SEGMENT_ID,
        CorruptRequestType::CHECKSUM
    };

    /**
     * @brief Vector representing the segment header fields.
     */
    const std::vector<CorruptRequestType::Field> segment_header_fields{
        CorruptRequestType::MAGIC,
        CorruptRequestType::VERSION,
        CorruptRequestType::SEGMENT_ID,
        CorruptRequestType::FIRST_TRANSACTION_OFFSET,
        CorruptRequestType::TXN_START_SEGMENT_ID,
        CorruptRequestType::REAL_TAIL_SEGMENT_ID,
        CorruptRequestType::GENERATION_ID,
        CorruptRequestType::CHECKSUM
    };

    /**
     * @brief Vector representing the transaction header fields.
     */
    const std::vector<CorruptRequestType::Field> transaction_header_fields{
        CorruptRequestType::MAGIC,
        CorruptRequestType::ID,
        CorruptRequestType::SEQUENCE_ID,
        CorruptRequestType::GENERATION_ID,
        CorruptRequestType::REMOVAL_COUNT,
        CorruptRequestType::ADD_COUNT,
        CorruptRequestType::TOTAL_ONDISK_NVO_HEADER_SIZE,
        CorruptRequestType::TOTAL_ONDISK_SIZE,
        CorruptRequestType::PADDING_SIZE,
        CorruptRequestType::HEADER_CHECKSUM,
        CorruptRequestType::PART_COUNT,
        CorruptRequestType::PART_CHECKSUMS
    };

    /** @brief Number of fields to check in ci tests. */
    static constexpr uint32_t CI_test_num_fields = 3;

public:
    TestWbMoyos()
        : RestartStack(obj::DspId(0, 1))
    {}

    void
    TestCaseSetUp() override
    {
        /*
         * Enable dbg-level logging when it's useful.
         * u_tr_set_threshold(MODULE_test, T_dbg);
         * u_tr_set_threshold(MODULE_writebuffer, T_dbg);
         */
        WbConfigData::set_config_enable_recovery(false);
        WbConfigData::set_config_try_all_possible_mirror_permutations(true);
        if (opt_legacy_copy_forward.is_set()) {
            tp_enable("writebuffer.enable.config.legacy.copy.forward");
        }

        /* Fetch a new DSP Internal Id from the Moyo Library */
        const PfPartitionId partition_id = moyo_service->new_dsp_internal_id();
        start_submodule_stack(partition_id);
        corruption_state = get_dsp_impl()->get_moyo_corruption_state();
    }

    void
    TestCaseTearDown() override
    {
        disable_corruption();
        stop_submodule_stack();
        corruption_state = nullptr;
    }

    static void
    SetUpTestSuite()
    {
        GTestObjPfCtrlVecFixture::SetUpTestSuite();
        moyo_service = dynamic_cast<NodeImpl*>(Node::get())->get_moyo_service();

        /* Initialize the AtmStorageArrayAllocatorPolicy, required for journal flush ops */
        size_t test_page_size = 1024;
        AtmStorageAryAllocatorPolicy::init(UScratchPool::get_allocator(),
                                           test_page_size);
    }

    static void
    TearDownTestSuite()
    {
        moyo_service = nullptr;
        GTestObjPfCtrlVecFixture::TearDownTestSuite();
    }

public: /* Functions */
    /**
     * @brief Fetch the PartitionId/DSP Internal Id from the DSP Uuid.
     *
     * @param[in] dsp_id The DSP id
     * @return partition_id
     */
    PfPartitionId dsp_to_partition_id(obj::DspId dsp_id);

    /**
     * @brief Cycle the submodule stack (stop/start)
     *
     * @param[in] validate_scratch_pool  Validate scratch page pool when
     *                                   stopping stack.
     * @param[in] partition_id           Partition id to assign the
     *                                   subcomponents, defaults to 0
     * @param[in] repair_component_id    Id of the component to repair.
     */
    void restart_submodule_stack(
        bool validate_scratch_pool = true,
        PfPartitionId partition_id = 0,
        PfComponentId repair_component_id = PfComponentId::Max) override;

    /**
     * @brief Add nvos to test transaction and commit it.
     *
     * @param[in] dsp       DSP for creating the test transaction.
     */
    void create_and_commit_transaction(DspImpl* dsp);

protected: /* Data members */
    /** @brief MoyoImpl object. */
    static MoyoImpl* moyo_service;

    /** @brief Server Context. */
    ServerContext ctx;

    /** @brief Corruption state for dsp. */
    DspImpl::MoyoInjectCorruptionState* corruption_state;
};

/* Tests all except corruption moyos */
using Phase0TestWbMoyos = TestWbMoyos;

MoyoImpl* TestWbMoyos::moyo_service = nullptr;

void
TestWbMoyos::fill_journal(std::vector<NvObjectBase*>* out_nvos,
                          size_t segment_count /*=0*/)
{
    DspImpl* const dsp = get_dsp_impl();

    const size_t starting_nvo_count = out_nvos->size();
    if (segment_count == 0) {
        /* Fill journal up to last usable segment */
        const size_t usage = dsp->get_segments_in_use();
        segment_count = dsp->get_usable_segment_count() - usage;
    }
    for (uint32_t index = 0; index < segment_count; index++) {
        TransactionPointer transaction = dsp->create_transaction(OWNER_wb);
        Payload<PAYLOAD_segment_size> payload;
        NvObjectBase* nvo =
            transaction->create_object<TestNvo<PAYLOAD_segment_size>,
                                       Payload<PAYLOAD_segment_size>>(
                payload, MEMTAG(AllocNvo));
        out_nvos->push_back(nvo);

        CommitOpStatus commit_op_status;
        uint32_t ondisk_usage = 0;
        transaction->sync_commit(std::move(transaction),
                                 Atm::get_misc_task_id(),
                                 dsp->get_shutdown_aei(),
                                 &commit_op_status,
                                 &ondisk_usage);
        EXPECT_EQ(ondisk_usage, dsp->get_segment_size());

        /* Assume we won't exceed journal full when using this method */
        EXPECT_TRUE(commit_op_status.get_and_clear());
    }
    const size_t ending_nvo_count = out_nvos->size();
    ASSERT_EQ(ending_nvo_count - starting_nvo_count, segment_count);
}

void
TestWbMoyos::restart_submodule_stack(
    const bool validate_scratch_pool /*=true*/,
    PfPartitionId partition_id /*=0*/,
    const PfComponentId repair_component_id /*=PfComponentId::Max*/)
{
    /* Fetch a new DSP Internal Id from the Moyo Library */
    if (0 == partition_id) {
        partition_id = moyo_service->new_dsp_internal_id();
    }
    RestartStack::restart_submodule_stack(validate_scratch_pool,
                                          partition_id,
                                          repair_component_id);
    corruption_state = get_dsp_impl()->get_moyo_corruption_state();
}

PfPartitionId
TestWbMoyos::dsp_to_partition_id(obj::DspId dsp_id)
{
    const SubCtrlVector* wb_scv =
        CtrlVector::get()->get_submodule_by_uuid<SubCtrlVector>(dsp_id).get();
    EXPECT_NE(wb_scv, nullptr);
    return wb_scv->get_partition_id();
}

void
TestWbMoyos::create_and_commit_transaction(DspImpl* dsp)
{
    TransactionImplPointer transaction =
        create_transaction<TransactionImplPointer>(dsp);
    const size_t payload_size = 64 * 1024;
    for (unsigned i = 0; i < 5; i++) {
        Payload<payload_size> p;
        transaction->create_object<TestNvo<payload_size>, Payload<payload_size>>(
            p, MEMTAG(AllocNvo));
    }
    transaction->sync_commit(std::move(transaction), Atm::get_misc_task_id());
}

/*
 * Enables a test point that simulates having only the reserved space free
 * while dumping a single DSP's journal. The DSP should fail to dump the journal.
 */
TEST_F(Phase0TestWbMoyos, journal_dump_target_free_space)
{
    ::google::protobuf::UInt32Value* const pf_id =
        new ::google::protobuf::UInt32Value;
    pf_id->set_value(dsp_to_partition_id(get_dsp_id()));
    DspInternalId dsp_internal_id;
    dsp_internal_id.set_allocated_dsp_id(pf_id);
    tp_enable("writebuffer.dump_free_space_reserved");
    DumpJournalResponseType response;
    Status rpc_status =
        moyo_service->dumpJournal(&ctx, &dsp_internal_id, &response);

    /*
     * Since the free disk space is reserved and not enough to dump a journal,
     * we should receive an error (errno 28) ENOSPC no space left on device.
     */
    EXPECT_TRUE(rpc_status.ok());
    EXPECT_EQ(response.status().error(), MoyoErrorCode::Failed);
    EXPECT_EQ(response.status().text(), "Error dumping journal due to error 28");
    tp_disable("writebuffer.dump_free_space_reserved");
}