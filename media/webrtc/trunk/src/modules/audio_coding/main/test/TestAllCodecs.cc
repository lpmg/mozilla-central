/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "TestAllCodecs.h"

#include <stdio.h>
#include <string>

#include "gtest/gtest.h"

#include "audio_coding_module.h"
#include "audio_coding_module_typedefs.h"
#include "common_types.h"
#include "engine_configurations.h"
#include "testsupport/fileutils.h"
#include "trace.h"
#include "typedefs.h"
#include "utility.h"

// Description of the test:
// In this test we set up a one-way communication channel from a participant
// called "a" to a participant called "b".
// a -> channel_a_to_b -> b
//
// The test loops through all available mono codecs, encode at "a" sends over
// the channel, and decodes at "b".

namespace webrtc {

// Class for simulating packet handling.
TestPack::TestPack()
    : receiver_acm_(NULL),
      sequence_number_(0),
      timestamp_diff_(0),
      last_in_timestamp_(0),
      total_bytes_(0),
      payload_size_(0) {
}

TestPack::~TestPack() {
}

void TestPack::RegisterReceiverACM(AudioCodingModule* acm) {
  receiver_acm_ = acm;
  return;
}

int32_t TestPack::SendData(FrameType frame_type, uint8_t payload_type,
                           uint32_t timestamp, const uint8_t* payload_data,
                           uint16_t payload_size,
                           const RTPFragmentationHeader* fragmentation) {
  WebRtcRTPHeader rtp_info;
  int32_t status;

  rtp_info.header.markerBit = false;
  rtp_info.header.ssrc = 0;
  rtp_info.header.sequenceNumber = sequence_number_++;
  rtp_info.header.payloadType = payload_type;
  rtp_info.header.timestamp = timestamp;
  if (frame_type == kAudioFrameCN) {
    rtp_info.type.Audio.isCNG = true;
  } else {
    rtp_info.type.Audio.isCNG = false;
  }
  if (frame_type == kFrameEmpty) {
    // Skip this frame.
    return 0;
  }

  // Only run mono for all test cases.
  rtp_info.type.Audio.channel = 1;
  memcpy(payload_data_, payload_data, payload_size);

  status =  receiver_acm_->IncomingPacket(payload_data_, payload_size,
                                          rtp_info);

  payload_size_ = payload_size;
  timestamp_diff_ = timestamp - last_in_timestamp_;
  last_in_timestamp_ = timestamp;
  total_bytes_ += payload_size;
  return status;
}

uint16_t TestPack::payload_size() {
  return payload_size_;
}

uint32_t TestPack::timestamp_diff() {
  return timestamp_diff_;
}

void TestPack::reset_payload_size() {
  payload_size_ = 0;
}

TestAllCodecs::TestAllCodecs(int test_mode)
    : acm_a_(NULL),
      acm_b_(NULL),
      channel_a_to_b_(NULL),
      test_count_(0),
      packet_size_samples_(0),
      packet_size_bytes_(0) {
  // test_mode = 0 for silent test (auto test)
  test_mode_ = test_mode;
}

TestAllCodecs::~TestAllCodecs() {
  if (acm_a_ != NULL) {
    AudioCodingModule::Destroy(acm_a_);
    acm_a_ = NULL;
  }
  if (acm_b_ != NULL) {
    AudioCodingModule::Destroy(acm_b_);
    acm_b_ = NULL;
  }
  if (channel_a_to_b_ != NULL) {
    delete channel_a_to_b_;
    channel_a_to_b_ = NULL;
  }
}

void TestAllCodecs::Perform() {

  const std::string file_name =
      webrtc::test::ResourcePath("audio_coding/testfile32kHz", "pcm");
  infile_a_.Open(file_name, 32000, "rb");

  if (test_mode_ == 0) {
    WEBRTC_TRACE(kTraceStateInfo, kTraceAudioCoding, -1,
                 "---------- TestAllCodecs ----------");
  }

  acm_a_ = AudioCodingModule::Create(0);
  acm_b_ = AudioCodingModule::Create(1);

  acm_a_->InitializeReceiver();
  acm_b_->InitializeReceiver();

  uint8_t num_encoders = acm_a_->NumberOfCodecs();
  CodecInst my_codec_param;
  for (uint8_t n = 0; n < num_encoders; n++) {
    acm_b_->Codec(n, my_codec_param);
    acm_b_->RegisterReceiveCodec(my_codec_param);
  }

  // Create and connect the channel
  channel_a_to_b_ = new TestPack;
  acm_a_->RegisterTransportCallback(channel_a_to_b_);
  channel_a_to_b_->RegisterReceiverACM(acm_b_);

  // All codecs are tested for all allowed sampling frequencies, rates and
  // packet sizes.
#ifdef WEBRTC_CODEC_AMR
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  char codec_amr[] = "AMR";
  RegisterSendCodec('A', codec_amr, 8000, 4750, 160, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 4750, 320, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 4750, 480, 3);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 5150, 160, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 5150, 320, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 5150, 480, 3);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 5900, 160, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 5900, 320, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 5900, 480, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 6700, 160, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 6700, 320, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 6700, 480, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 7400, 160, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 7400, 320, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 7400, 480, 3);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 7950, 160, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 7950, 320, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 7950, 480, 3);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 10200, 160, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 10200, 320, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 10200, 480, 3);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 12200, 160, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 12200, 320, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amr, 8000, 12200, 480, 3);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
#ifdef WEBRTC_CODEC_AMRWB
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  char codec_amrwb[] = "AMR-WB";
  OpenOutFile(test_count_);
  RegisterSendCodec('A', codec_amrwb, 16000, 7000, 320, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 7000, 640, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 7000, 960, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 9000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 9000, 640, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 9000, 960, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 12000, 320, 3);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 12000, 640, 6);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 12000, 960, 8);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 14000, 320, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 14000, 640, 4);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 14000, 960, 5);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 16000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 16000, 640, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 16000, 960, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 18000, 320, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 18000, 640, 4);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 18000, 960, 5);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 20000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 20000, 640, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 20000, 960, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 23000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 23000, 640, 3);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 23000, 960, 3);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 24000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 24000, 640, 2);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_amrwb, 16000, 24000, 960, 2);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
#ifdef WEBRTC_CODEC_G722
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  char codec_g722[] = "G722";
  RegisterSendCodec('A', codec_g722, 16000, 64000, 160, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g722, 16000, 64000, 320, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g722, 16000, 64000, 480, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g722, 16000, 64000, 640, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g722, 16000, 64000, 800, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g722, 16000, 64000, 960, 0);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
#ifdef WEBRTC_CODEC_G722_1
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  char codec_g722_1[] = "G7221";
  RegisterSendCodec('A', codec_g722_1, 16000, 32000, 320, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g722_1, 16000, 24000, 320, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g722_1, 16000, 16000, 320, 0);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
#ifdef WEBRTC_CODEC_G722_1C
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  char codec_g722_1c[] = "G7221";
  RegisterSendCodec('A', codec_g722_1c, 32000, 48000, 640, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g722_1c, 32000, 32000, 640, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g722_1c, 32000, 24000, 640, 0);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
#ifdef WEBRTC_CODEC_G729
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  char codec_g729[] = "G729";
  RegisterSendCodec('A', codec_g729, 8000, 8000, 80, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729, 8000, 8000, 160, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729, 8000, 8000, 240, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729, 8000, 8000, 320, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729, 8000, 8000, 400, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729, 8000, 8000, 480, 0);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
#ifdef WEBRTC_CODEC_G729_1
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  char codec_g729_1[] = "G7291";
  RegisterSendCodec('A', codec_g729_1, 16000, 8000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 8000, 640, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 8000, 960, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 12000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 12000, 640, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 12000, 960, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 14000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 14000, 640, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 14000, 960, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 16000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 16000, 640, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 16000, 960, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 18000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 18000, 640, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 18000, 960, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 20000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 20000, 640, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 20000, 960, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 22000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 22000, 640, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 22000, 960, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 24000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 24000, 640, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 24000, 960, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 26000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 26000, 640, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 26000, 960, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 28000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 28000, 640, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 28000, 960, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 30000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 30000, 640, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 30000, 960, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 32000, 320, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 32000, 640, 1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_g729_1, 16000, 32000, 960, 1);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
#ifdef WEBRTC_CODEC_GSMFR
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  char codec_gsmfr[] = "GSM";
  RegisterSendCodec('A', codec_gsmfr, 8000, 13200, 160, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_gsmfr, 8000, 13200, 320, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_gsmfr, 8000, 13200, 480, 0);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
#ifdef WEBRTC_CODEC_ILBC
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  char codec_ilbc[] = "ILBC";
  RegisterSendCodec('A', codec_ilbc, 8000, 13300, 240, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_ilbc, 8000, 13300, 480, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_ilbc, 8000, 15200, 160, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_ilbc, 8000, 15200, 320, 0);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
#if (defined(WEBRTC_CODEC_ISAC) || defined(WEBRTC_CODEC_ISACFX))
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  char codec_isac[] = "ISAC";
  RegisterSendCodec('A', codec_isac, 16000, -1, 480, -1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_isac, 16000, -1, 960, -1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_isac, 16000, 15000, 480, -1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_isac, 16000, 32000, 960, -1);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
#ifdef WEBRTC_CODEC_ISAC
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  RegisterSendCodec('A', codec_isac, 32000, -1, 960, -1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_isac, 32000, 56000, 960, -1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_isac, 32000, 37000, 960, -1);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_isac, 32000, 32000, 960, -1);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
#ifdef WEBRTC_CODEC_PCM16
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  char codec_l16[] = "L16";
  RegisterSendCodec('A', codec_l16, 8000, 128000, 80, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_l16, 8000, 128000, 160, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_l16, 8000, 128000, 240, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_l16, 8000, 128000, 320, 0);
  Run(channel_a_to_b_);
  outfile_b_.Close();
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  RegisterSendCodec('A', codec_l16, 16000, 256000, 160, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_l16, 16000, 256000, 320, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_l16, 16000, 256000, 480, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_l16, 16000, 256000, 640, 0);
  Run(channel_a_to_b_);
  outfile_b_.Close();
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  RegisterSendCodec('A', codec_l16, 32000, 512000, 320, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_l16, 32000, 512000, 640, 0);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  char codec_pcma[] = "PCMA";
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 80, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 160, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 240, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 320, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 400, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_pcma, 8000, 64000, 480, 0);
  Run(channel_a_to_b_);
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  char codec_pcmu[] = "PCMU";
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 80, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 160, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 240, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 320, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 400, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_pcmu, 8000, 64000, 480, 0);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#ifdef WEBRTC_CODEC_SPEEX
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  char codec_speex[] = "SPEEX";
  RegisterSendCodec('A', codec_speex, 8000, 2400, 160, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_speex, 8000, 8000, 320, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_speex, 8000, 18200, 480, 0);
  Run(channel_a_to_b_);
  outfile_b_.Close();

  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  RegisterSendCodec('A', codec_speex, 16000, 4000, 320, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_speex, 16000, 12800, 640, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_speex, 16000, 34200, 960, 0);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
#ifdef WEBRTC_CODEC_CELT
  if (test_mode_ != 0) {
    printf("===============================================================\n");
  }
  test_count_++;
  OpenOutFile(test_count_);
  char codec_celt[] = "CELT";
  RegisterSendCodec('A', codec_celt, 32000, 48000, 640, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_celt, 32000, 64000, 640, 0);
  Run(channel_a_to_b_);
  RegisterSendCodec('A', codec_celt, 32000, 128000, 640, 0);
  Run(channel_a_to_b_);
  outfile_b_.Close();
#endif
  if (test_mode_ != 0) {
    printf("===============================================================\n");

    /* Print out all codecs that were not tested in the run */
    printf("The following codecs was not included in the test:\n");
#ifndef WEBRTC_CODEC_AMR
    printf("   GSMAMR\n");
#endif
#ifndef WEBRTC_CODEC_AMRWB
    printf("   GSMAMR-wb\n");
#endif
#ifndef WEBRTC_CODEC_G722
    printf("   G.722\n");
#endif
#ifndef WEBRTC_CODEC_G722_1
    printf("   G.722.1\n");
#endif
#ifndef WEBRTC_CODEC_G722_1C
    printf("   G.722.1C\n");
#endif
#ifndef WEBRTC_CODEC_G729
    printf("   G.729\n");
#endif
#ifndef WEBRTC_CODEC_G729_1
    printf("   G.729.1\n");
#endif
#ifndef WEBRTC_CODEC_GSMFR
    printf("   GSMFR\n");
#endif
#ifndef WEBRTC_CODEC_ILBC
    printf("   iLBC\n");
#endif
#ifndef WEBRTC_CODEC_ISAC
    printf("   ISAC float\n");
#endif
#ifndef WEBRTC_CODEC_ISACFX
    printf("   ISAC fix\n");
#endif
#ifndef WEBRTC_CODEC_PCM16
    printf("   PCM16\n");
#endif
#ifndef WEBRTC_CODEC_SPEEX
    printf("   Speex\n");
#endif

    printf("\nTo complete the test, listen to the %d number of output files.\n",
           test_count_);
  }
}

// Register Codec to use in the test
//
// Input:  side             - which ACM to use, 'A' or 'B'
//         codec_name       - name to use when register the codec
//         sampling_freq_hz - sampling frequency in Herz
//         rate             - bitrate in bytes
//         packet_size      - packet size in samples
//         extra_byte       - if extra bytes needed compared to the bitrate
//                            used when registering, can be an internal header
//                            set to -1 if the codec is a variable rate codec
void TestAllCodecs::RegisterSendCodec(char side, char* codec_name,
                                      int32_t sampling_freq_hz, int rate,
                                      int packet_size, int extra_byte) {
  if (test_mode_ != 0) {
    // Print out codec and settings.
    printf("codec: %s Freq: %d Rate: %d PackSize: %d\n", codec_name,
           sampling_freq_hz, rate, packet_size);
  }

  // Store packet-size in samples, used to validate the received packet.
  // If G.722, store half the size to compensate for the timestamp bug in the
  // RFC for G.722.
  // If iSAC runs in adaptive mode, packet size in samples can change on the
  // fly, so we exclude this test by setting |packet_size_samples_| to -1.
  if (!strcmp(codec_name, "G722")) {
    packet_size_samples_ = packet_size / 2;
  } else if (!strcmp(codec_name, "ISAC") && (rate == -1)) {
    packet_size_samples_ = -1;
  } else {
    packet_size_samples_ = packet_size;
  }

  // Store the expected packet size in bytes, used to validate the received
  // packet. If variable rate codec (extra_byte == -1), set to -1 (65535).
  if (extra_byte != -1) {
    // Add 0.875 to always round up to a whole byte
    packet_size_bytes_ =
        static_cast<uint16_t>(static_cast<float>(packet_size * rate) /
        static_cast<float>(sampling_freq_hz * 8) + 0.875) + extra_byte;
  } else {
    // Packets will have a variable size.
    packet_size_bytes_ = -1;
  }

  // Set pointer to the ACM where to register the codec.
  AudioCodingModule* my_acm = NULL;
  switch (side) {
    case 'A': {
      my_acm = acm_a_;
      break;
    }
    case 'B': {
      my_acm = acm_b_;
      break;
    }
    default: {
      break;
    }
  }
  ASSERT_TRUE(my_acm != NULL);

  // Get all codec parameters before registering
  CodecInst my_codec_param;
  CHECK_ERROR(AudioCodingModule::Codec(codec_name, my_codec_param,
                                       sampling_freq_hz, 1));
  my_codec_param.rate = rate;
  my_codec_param.pacsize = packet_size;
  CHECK_ERROR(my_acm->RegisterSendCodec(my_codec_param));
}

void TestAllCodecs::Run(TestPack* channel) {
  AudioFrame audio_frame;

  int32_t out_freq_hz = outfile_b_.SamplingFrequency();
  uint16_t receive_size;
  uint32_t timestamp_diff;
  channel->reset_payload_size();
  int error_count = 0;

  int counter = 0;
  while (!infile_a_.EndOfFile()) {
    // Add 10 msec to ACM.
    infile_a_.Read10MsData(audio_frame);
    CHECK_ERROR(acm_a_->Add10MsData(audio_frame));

    // Run sender side of ACM.
    CHECK_ERROR(acm_a_->Process());

    // Verify that the received packet size matches the settings.
    receive_size = channel->payload_size();
    if (receive_size) {
      if ((receive_size != packet_size_bytes_) &&
          (packet_size_bytes_ < 65535)) {
        error_count++;
      }

      // Verify that the timestamp is updated with expected length. The counter
      // is used to avoid problems when switching codec or frame size in the
      // test.
      timestamp_diff = channel->timestamp_diff();
      if ((counter > 10) && (timestamp_diff != packet_size_samples_) &&
          (packet_size_samples_ < 65535))
        error_count++;
    }

    // Run received side of ACM.
    CHECK_ERROR(acm_b_->PlayoutData10Ms(out_freq_hz, audio_frame));

    // Write output speech to file.
    outfile_b_.Write10MsData(audio_frame.data_,
                             audio_frame.samples_per_channel_);

    // Update loop counter
    counter++;
  }

  EXPECT_EQ(0, error_count);

  if (infile_a_.EndOfFile()) {
    infile_a_.Rewind();
  }
}

void TestAllCodecs::OpenOutFile(int test_number) {
  std::string filename = webrtc::test::OutputPath();
  std::ostringstream test_number_str;
  test_number_str << test_number;
  filename += "testallcodecs_out_";
  filename += test_number_str.str();
  filename += ".pcm";
  outfile_b_.Open(filename, 32000, "wb");
}

void TestAllCodecs::DisplaySendReceiveCodec() {
  CodecInst my_codec_param;
  acm_a_->SendCodec(my_codec_param);
  printf("%s -> ", my_codec_param.plname);
  acm_b_->ReceiveCodec(my_codec_param);
  printf("%s\n", my_codec_param.plname);
}

}  // namespace webrtc

