/**
 * @file ltf_moyos.cc
 * @brief Contains the implementation of `MoyoImpl` methods and related code.
 */

#define __MODULE__ MODULE_writebuffer

#include <boost/tokenizer.hpp>

#include "atm_sync_op.h"
#include "clustercfg_node.h"
#include "raid_sub_ctrl_vector.h"
#include "wb_alloc.h"
#include "wb_common_nvos.h"
#include "wb_copy_forward.h"
#include "wb_ctrl_vector.h"
#include "wb_disk_sync.h"
#include "wb_journal.h"
#include "wb_moyos.h"
#include "wb_node_impl.h"
#include "wb_raid_io.h"
#include "wb_sub_ctrl_vector_impl.h"
#include "wb_superblock.h"
#include "wb_transaction_commit.h"
#include "wb_transaction_impl.h"
#include "wb_types.h"
#include "WbConfigData.h"

namespace ltf {

/**
 * @brief Test Nvo Types
 * @todo  Update when new types are supported
 */
std::unordered_map<std::string, NvObjectType> test_nvo_types = {
    { "Test1", NvObjectType::Test1 },
    { "Test2", NvObjectType::Test2 },
    { "Test3", NvObjectType::Test3 },
    { "Test4", NvObjectType::Test4 }
};

/**
 * @typedef CommitOpStatusPointer
 *
 * @brief CommitOpStatus unique pointer.
 */
typedef std::unique_ptr<CommitOpStatus> CommitOpStatusPointer;

#include "ltf_moyo.pb.h"
using namespace ltf;

Status MoyoImpl::ltf_trace_record_enable(ServerContext* context,
                                      const LtfTraceRecordEnableRequest* request,
                                      LtfTraceRecordEnableResponse* response) override {
    // Check if the file path is valid and the user has permissions to write to it
    if (!isValidFilePath(request->file_path())) {
        response->mutable_status()->set_error(MoyoErrorCode::Failed);
        response->mutable_status()->set_text("Invalid file path");
        return Status::OK;
    }

    // Prevents multiple concurrent recordings to same file
    if (isRecording(request->file_path())) {
        response->mutable_status()->set_error(MoyoErrorCode::Failed);
        response->mutable_status()->set_text("Already recording to this file");
        return Status::OK;
    }

    // Start recording
    startRecording(request->file_path(), request->sampling_rate(), request->duration_seconds());

    response->mutable_status()->set_error(MoyoErrorCode::Success);
    response->mutable_status()->set_text("Trace recording started");

    return Status::OK;
}

bool MoyoImpl::isValidFilePath(const std::string& file_path) {
    // Implement your file path validation logic here.
    // You might want to check if the path is valid, if the user has write permissions, etc.
    return true;
}

bool MoyoImpl::isRecording(const std::string& file_path) {
    // Implement your logic to check if a recording is already in progress for this file.
    return false;
}

void MoyoImpl::startRecording(const std::string& file_path, float sampling_rate, int duration_seconds) {
    // Implement your logic to start recording traces to the specified file.
    // You might want to use a timer to stop the recording after the specified duration.
}

} /* namespace writebuffer */
