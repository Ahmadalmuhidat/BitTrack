#include "../include/remote.hpp"
#include "../include/config.hpp"
#include <fstream>
#include <gtest/gtest.h>

TEST(RemoteTest, ListRemoteBranchesReturnsEmptyForInvalidRemote) {
  std::vector<std::string> branches = listRemoteBranches("invalid_remote");
  EXPECT_TRUE(branches.empty());
}

TEST(RemoteTest, DeleteRemoteBranchReturnsFalseForInvalidRemote) {
  bool result = deleteRemoteBranch("some_branch", "invalid_remote");
  EXPECT_FALSE(result);
}

TEST(RemoteTest, PushWithInvalidRemoteURL) {
  EXPECT_NO_THROW(push("invalid_url", "main"));
}

TEST(RemoteTest, Base64DecodeValid) {
  std::string encoded = "SGVsbG8gV29ybGQ=";
  std::string decoded = base64Decode(encoded);
  EXPECT_EQ(decoded, "Hello World");
}

TEST(RemoteTest, Base64DecodeWithNewlines) {
  // Current base64Decode skips real newlines
  std::string encoded = "SGVsbG8g\nV29ybGQ=";
  std::string decoded = base64Decode(encoded);
  EXPECT_EQ(decoded, "Hello World");
}

// This test confirms that raw JSON escapes would fail if not stripped
// But since we stripped them in the caller, this test just confirms
// base64Decode behavior. If we passed "SGVsbG8g\\nV29ybGQ=", base64Decode
// would print error and return empty.
TEST(RemoteTest, Base64DecodeFailsOnBackslash) {
  // This expects the error log to be printed and return empty string
  std::string encoded = "SGVsbG8g\\nV29ybGQ=";
  std::string decoded = base64Decode(encoded);
  EXPECT_EQ(decoded, "");
}
TEST(RemoteTest, PullWithInvalidRemoteURL) {
  EXPECT_NO_THROW(pull("invalid_url", "main"));
}
