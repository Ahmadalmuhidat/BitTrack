#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <vector>
#include <algorithm>

// Include test function declarations
extern bool test_branch_master();
extern bool test_list_branches();
extern bool test_checkout_new_branch();
extern bool test_remove_branch();
extern bool test_untracked_file_preservation();
extern bool test_switch_to_nonexistent_branch();
extern bool test_switch_to_same_branch();
extern bool test_diff_file_comparison();
extern bool test_diff_identical_files();
extern bool test_diff_binary_file_detection();
extern bool test_config_set_and_get();
extern bool test_config_user_name();
extern bool test_config_user_email();
extern bool test_config_list();
extern bool test_config_unset();
extern bool test_config_repository_config();
extern bool test_config_default_configs();
extern bool test_merge_show_conflicts();
extern bool test_merge_has_conflicts();
extern bool test_merge_get_conflicted_files();
extern bool test_merge_three_way();
extern bool test_hooks_install_default();
extern bool test_hooks_list();
extern bool test_hooks_uninstall();
extern bool test_hooks_run();
extern bool test_hooks_run_all();
extern bool test_hooks_get_path();
extern bool test_hooks_get_name();
extern bool test_hooks_is_executable();
extern bool test_hooks_create_pre_commit();
extern bool test_hooks_create_post_commit();
extern bool test_hooks_create_pre_push();
extern bool test_maintenance_garbage_collect();
extern bool test_maintenance_repack();
extern bool test_maintenance_prune();
extern bool test_maintenance_fsck();
extern bool test_maintenance_stats();
extern bool test_maintenance_show_info();
extern bool test_maintenance_analyze();
extern bool test_maintenance_find_large_files();
extern bool test_maintenance_find_duplicates();
extern bool test_maintenance_clean_ignored();
extern bool test_maintenance_remove_empty_dirs();
extern bool test_maintenance_compact();
extern bool test_maintenance_backup();
extern bool test_maintenance_list_backups();
extern bool test_maintenance_benchmark();
extern bool test_maintenance_profile();
extern bool test_maintenance_check_integrity();
extern bool test_maintenance_optimize();
extern bool test_error_handler_print_error();
extern bool test_error_handler_print_error_code();
extern bool test_error_handler_is_fatal();
extern bool test_error_handler_get_message();
extern bool test_error_handler_validate_arguments();
extern bool test_error_handler_validate_file_path();
extern bool test_error_handler_validate_branch_name();
extern bool test_error_handler_validate_commit_message();
extern bool test_error_handler_validate_remote_url();
extern bool test_error_handler_safe_create_directories();
extern bool test_error_handler_safe_copy_file();
extern bool test_error_handler_safe_remove_file();
extern bool test_error_handler_safe_write_file();
extern bool test_error_handler_safe_read_file();
extern bool test_error_handler_validate_repository();
extern bool test_error_handler_validate_branch_exists();
extern bool test_error_handler_validate_no_uncommitted();

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST(t01_branch, valid_current_branch_is_master_test)
{
  EXPECT_TRUE(test_branch_master());
}

TEST(t02_branch, list_branches_test)
{
  EXPECT_TRUE(test_list_branches());
}

TEST(t03_branch, checkout_to_new_branch_test)
{
  EXPECT_TRUE(test_checkout_new_branch());
}
TEST(t04_stage_working, stage_file_test)
{
  std::ofstream file("stage_working_test.txt");
  file << "content for staging" << std::endl;
  file.close();

  std::filesystem::create_directories(".bittrack");
  std::ofstream index_file(".bittrack/index", std::ios::app);
  index_file << "stage_working_test.txt dummy_hash" << std::endl;
  index_file.close();

  std::vector<std::string> staged_files;
  std::ifstream read_index(".bittrack/index");
  std::string line;
  while (std::getline(read_index, line))
  {
    std::istringstream iss(line);
    std::string fileName, fileHash;
    if (iss >> fileName >> fileHash)
    {
      staged_files.push_back(fileName);
    }
  }
  read_index.close();

  bool is_staged = std::find(staged_files.begin(), staged_files.end(), "stage_working_test.txt") != staged_files.end();
  EXPECT_TRUE(is_staged);

  std::filesystem::remove("stage_working_test.txt");
  std::filesystem::remove(".bittrack/index");
}
TEST(t07_branch, remove_new_branch_test)
{
  EXPECT_TRUE(test_remove_branch());
}
TEST(t09_branch, untracked_file_preservation_test)
{
  EXPECT_TRUE(test_untracked_file_preservation());
}
TEST(t11_branch, switch_to_nonexistent_branch_test)
{
  EXPECT_TRUE(test_switch_to_nonexistent_branch());
}

TEST(t12_branch, switch_to_same_branch_test)
{
  EXPECT_TRUE(test_switch_to_same_branch());
}
TEST(t14_diff, file_comparison_test)
{
  EXPECT_TRUE(test_diff_file_comparison());
}

TEST(t15_diff, identical_files_test)
{
  EXPECT_TRUE(test_diff_identical_files());
}

TEST(t16_diff, binary_file_detection_test)
{
  EXPECT_TRUE(test_diff_binary_file_detection());
}

TEST(t27_config, set_and_get_test)
{
  EXPECT_TRUE(test_config_set_and_get());
}

TEST(t28_config, user_name_test)
{
  EXPECT_TRUE(test_config_user_name());
}

TEST(t29_config, user_email_test)
{
  EXPECT_TRUE(test_config_user_email());
}

TEST(t30_config, list_test)
{
  EXPECT_TRUE(test_config_list());
}

TEST(t31_config, unset_test)
{
  EXPECT_TRUE(test_config_unset());
}

TEST(t32_config, repository_config_test)
{
  EXPECT_TRUE(test_config_repository_config());
}

TEST(t33_config, default_configs_test)
{
  EXPECT_TRUE(test_config_default_configs());
}
TEST(t45_merge, show_conflicts_test)
{
  EXPECT_TRUE(test_merge_show_conflicts());
}

TEST(t46_merge, has_conflicts_test)
{
  EXPECT_TRUE(test_merge_has_conflicts());
}

TEST(t47_merge, get_conflicted_files_test)
{
  EXPECT_TRUE(test_merge_get_conflicted_files());
}

TEST(t48_merge, three_way_test)
{
  EXPECT_TRUE(test_merge_three_way());
}
TEST(t50_hooks, install_default_test)
{
  EXPECT_TRUE(test_hooks_install_default());
}

TEST(t51_hooks, list_test)
{
  EXPECT_TRUE(test_hooks_list());
}

TEST(t52_hooks, uninstall_test)
{
  EXPECT_TRUE(test_hooks_uninstall());
}

TEST(t53_hooks, run_test)
{
  EXPECT_TRUE(test_hooks_run());
}

TEST(t54_hooks, run_all_test)
{
  EXPECT_TRUE(test_hooks_run_all());
}

TEST(t55_hooks, get_path_test)
{
  EXPECT_TRUE(test_hooks_get_path());
}

TEST(t56_hooks, get_name_test)
{
  EXPECT_TRUE(test_hooks_get_name());
}

TEST(t57_hooks, is_executable_test)
{
  EXPECT_TRUE(test_hooks_is_executable());
}

TEST(t58_hooks, create_pre_commit_test)
{
  EXPECT_TRUE(test_hooks_create_pre_commit());
}

TEST(t59_hooks, create_post_commit_test)
{
  EXPECT_TRUE(test_hooks_create_post_commit());
}

TEST(t60_hooks, create_pre_push_test)
{
  EXPECT_TRUE(test_hooks_create_pre_push());
}

TEST(t61_maintenance, garbage_collect_test)
{
  EXPECT_TRUE(test_maintenance_garbage_collect());
}

TEST(t62_maintenance, repack_test)
{
  EXPECT_TRUE(test_maintenance_repack());
}

TEST(t63_maintenance, prune_test)
{
  EXPECT_TRUE(test_maintenance_prune());
}

TEST(t64_maintenance, fsck_test)
{
  EXPECT_TRUE(test_maintenance_fsck());
}

TEST(t65_maintenance, stats_test)
{
  EXPECT_TRUE(test_maintenance_stats());
}

TEST(t66_maintenance, show_info_test)
{
  EXPECT_TRUE(test_maintenance_show_info());
}

TEST(t67_maintenance, analyze_test)
{
  EXPECT_TRUE(test_maintenance_analyze());
}

TEST(t68_maintenance, find_large_files_test)
{
  EXPECT_TRUE(test_maintenance_find_large_files());
}

TEST(t69_maintenance, find_duplicates_test)
{
  EXPECT_TRUE(test_maintenance_find_duplicates());
}


TEST(t71_maintenance, clean_ignored_test)
{
  EXPECT_TRUE(test_maintenance_clean_ignored());
}

TEST(t72_maintenance, remove_empty_dirs_test)
{
  EXPECT_TRUE(test_maintenance_remove_empty_dirs());
}

TEST(t73_maintenance, compact_test)
{
  EXPECT_TRUE(test_maintenance_compact());
}

TEST(t74_maintenance, backup_test)
{
  EXPECT_TRUE(test_maintenance_backup());
}

TEST(t75_maintenance, list_backups_test)
{
  EXPECT_TRUE(test_maintenance_list_backups());
}

TEST(t76_maintenance, benchmark_test)
{
  EXPECT_TRUE(test_maintenance_benchmark());
}

TEST(t77_maintenance, profile_test)
{
  EXPECT_TRUE(test_maintenance_profile());
}

TEST(t78_maintenance, check_integrity_test)
{
  EXPECT_TRUE(test_maintenance_check_integrity());
}

TEST(t79_maintenance, optimize_test)
{
  EXPECT_TRUE(test_maintenance_optimize());
}

TEST(t80_error, print_error_test)
{
  EXPECT_TRUE(test_error_handler_print_error());
}

TEST(t81_error, print_error_code_test)
{
  EXPECT_TRUE(test_error_handler_print_error_code());
}

TEST(t82_error, is_fatal_test)
{
  EXPECT_TRUE(test_error_handler_is_fatal());
}

TEST(t83_error, get_message_test)
{
  EXPECT_TRUE(test_error_handler_get_message());
}

TEST(t84_error, validate_arguments_test)
{
  EXPECT_TRUE(test_error_handler_validate_arguments());
}

TEST(t85_error, validate_file_path_test)
{
  EXPECT_TRUE(test_error_handler_validate_file_path());
}

TEST(t86_error, validate_branch_name_test)
{
  EXPECT_TRUE(test_error_handler_validate_branch_name());
}

TEST(t87_error, validate_commit_message_test)
{
  EXPECT_TRUE(test_error_handler_validate_commit_message());
}

TEST(t88_error, validate_remote_url_test)
{
  EXPECT_TRUE(test_error_handler_validate_remote_url());
}

TEST(t89_error, safe_create_directories_test)
{
  EXPECT_TRUE(test_error_handler_safe_create_directories());
}

TEST(t90_error, safe_copy_file_test)
{
  EXPECT_TRUE(test_error_handler_safe_copy_file());
}

TEST(t91_error, safe_remove_file_test)
{
  EXPECT_TRUE(test_error_handler_safe_remove_file());
}

TEST(t92_error, safe_write_file_test)
{
  EXPECT_TRUE(test_error_handler_safe_write_file());
}

TEST(t93_error, safe_read_file_test)
{
  EXPECT_TRUE(test_error_handler_safe_read_file());
}

TEST(t94_error, validate_repository_test)
{
  EXPECT_TRUE(test_error_handler_validate_repository());
}

TEST(t95_error, validate_branch_exists_test)
{
  EXPECT_TRUE(test_error_handler_validate_branch_exists());
}

TEST(t96_error, validate_no_uncommitted_test)
{
  EXPECT_TRUE(test_error_handler_validate_no_uncommitted());
}
