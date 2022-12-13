// Copyright 2017 The CrunchyCrypt Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "crunchy/internal/keys/macing_key.h"

#include <stddef.h>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "crunchy/internal/algs/mac/openssl_hmac.h"
#include "crunchy/internal/algs/random/crypto_rand.h"
#include "crunchy/internal/common/init.h"
#include "crunchy/internal/common/status_matchers.h"
#include "crunchy/internal/common/test_factory.h"
#include "crunchy/internal/keys/macing_key_test_vectors.pb.h"

namespace crunchy {

namespace {

std::vector<FactoryInfo<MacingKeyFactory>>* FactoryInfoVector() {
  auto factories = new std::vector<FactoryInfo<MacingKeyFactory>>();
  static const MacingKeyFactory& hmac_sha256_factory =
      *MakeFactory(GetHmacSha256HalfDigestFactory()).release();
  factories->push_back(
      {"hmac_sha256", hmac_sha256_factory,
       "crunchy/internal/keys/testdata/hmac_sha256.proto.bin"});
  return factories;
}

using MacingKeyTest = FactoryParamTest<MacingKeyFactory, FactoryInfoVector>;

TEST_P(MacingKeyTest, SignVerify) {
  KeyData key_data = factory().CreateRandomKeyData();

  std::string message = "banana";
  auto status_or_key = factory().MakeKey(key_data);
  CRUNCHY_EXPECT_OK(status_or_key.status());
  std::unique_ptr<MacingKey> key = std::move(status_or_key.ValueOrDie());

  auto status_or_signature = key->Sign(message);
  CRUNCHY_EXPECT_OK(status_or_signature.status());
  std::string signature = std::move(status_or_signature.ValueOrDie());

  CRUNCHY_EXPECT_OK(key->Verify(message, signature));
}

TEST_P(MacingKeyTest, SignVerifyErrors) {
  KeyData key_data = factory().CreateRandomKeyData();

  std::string message = "banana";
  auto status_or_key = factory().MakeKey(key_data);
  CRUNCHY_EXPECT_OK(status_or_key.status());
  std::unique_ptr<MacingKey> key = std::move(status_or_key.ValueOrDie());

  auto status_or_signature = key->Sign(message);
  CRUNCHY_EXPECT_OK(status_or_signature.status());
  std::string signature = std::move(status_or_signature.ValueOrDie());

  CRUNCHY_EXPECT_OK(key->Verify(message, signature));

  // Corrupt signature start
  signature[0] ^= 0x01;
  EXPECT_FALSE(key->Verify(message, signature).ok());
  signature[0] ^= 0x01;

  // Corrupt ciphertext middle
  signature[signature.length() / 2] ^= 0x01;
  EXPECT_FALSE(key->Verify(message, signature).ok());
  signature[signature.length() / 2] ^= 0x01;

  // Corrupt signature end
  signature[signature.length() - 1] ^= 0x01;
  EXPECT_FALSE(key->Verify(message, signature).ok());
  signature[signature.length() - 1] ^= 0x01;

  // Corrupt message
  message[0] ^= 0x01;
  EXPECT_FALSE(key->Verify(message, signature).ok());
  message[0] ^= 0x01;

  // Signature too short
  EXPECT_FALSE(
      key->Verify(message, absl::ClippedSubstr(absl::string_view(signature),
                                               signature.length() - 1))
          .ok());
}

TEST_P(MacingKeyTest, BadKeyData) {
  KeyData key_data = factory().CreateRandomKeyData();

  auto status_or_key = factory().MakeKey(key_data);
  CRUNCHY_EXPECT_OK(status_or_key.status());
  std::unique_ptr<MacingKey> key = std::move(status_or_key.ValueOrDie());

  // MakeSigner with missing key
  KeyData bad_key_data = key_data;
  bad_key_data.clear_private_key();
  EXPECT_FALSE(factory().MakeKey(bad_key_data).ok());

  // MakeSigner with corrupt key
  bad_key_data = key_data;
  bad_key_data.set_private_key("corn");
  EXPECT_FALSE(factory().MakeKey(bad_key_data).ok());
}

void VerifyTestVector(const MacingKeyFactory& factory,
                      const MacingKeyTestVector& test_vector) {
  // Create a signer from the test_vector
  auto status_or_key = factory.MakeKey(test_vector.key_data());
  CRUNCHY_EXPECT_OK(status_or_key.status());
  std::unique_ptr<MacingKey> key = std::move(status_or_key.ValueOrDie());

  // Verify the test vector
  CRUNCHY_EXPECT_OK(
      key->Verify(test_vector.message(), test_vector.signature()));

  // Sign and verify the message
  auto status_or_signature = key->Sign(test_vector.message());
  CRUNCHY_EXPECT_OK(status_or_signature.status());
  std::string signature = std::move(status_or_signature.ValueOrDie());
  CRUNCHY_EXPECT_OK(key->Verify(test_vector.message(), signature));
}

TEST_P(MacingKeyTest, TestVectors) {
  auto test_vectors = GetTestVectors<MacingKeyTestVectors>();
  for (const auto& test_vector : test_vectors.test_vector()) {
    VerifyTestVector(factory(), test_vector);
  }
}

INSTANTIATE_TEST_CASE_P(, MacingKeyTest,
                        ::testing::ValuesIn(MacingKeyTest::factories()),
                        MacingKeyTest::GetNameFromParam);

MacingKeyTestVector GenerateTestVector(const MacingKeyFactory& factory) {
  KeyData key_data = factory.CreateRandomKeyData();

  size_t message_magnatude = BiasRandInt(10);
  size_t message_length = BiasRandInt(1 << message_magnatude);
  std::string message = RandString(message_length);

  auto status_or_key = factory.MakeKey(key_data);
  CRUNCHY_EXPECT_OK(status_or_key.status());
  std::unique_ptr<MacingKey> key = std::move(status_or_key.ValueOrDie());

  auto status_or_signature = key->Sign(message);
  CRUNCHY_EXPECT_OK(status_or_signature.status());
  std::string signature = std::move(status_or_signature.ValueOrDie());

  MacingKeyTestVector test_vector;
  *test_vector.mutable_key_data() = key_data;
  test_vector.set_message(message);
  test_vector.set_signature(signature);

  VerifyTestVector(factory, test_vector);
  return test_vector;
}

}  // namespace

}  // namespace crunchy

int main(int argc, char** argv) {
  crunchy::InitCrunchyTest(argv[0], &argc, &argv, true);
  return RUN_ALL_TESTS();
}
