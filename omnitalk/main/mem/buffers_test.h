#pragma once

#include "mem/buffers.h"
#include "test.h"

buffer_t* buf_from_string(char* str, size_t l2_hdr_length, size_t frame_length);

TEST_FUNCTION(test_newbuf);
TEST_FUNCTION(test_buf_l2hdr_shenanigans);
TEST_FUNCTION(test_buf_ddp_setup);
TEST_FUNCTION(test_buf_append);
