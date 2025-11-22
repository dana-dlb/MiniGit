import unittest
import subprocess
import shutil
import os
import json


def remove_repository():
    dir_path = ".minigit";
    if os.path.exists(dir_path):
        shutil.rmtree(dir_path)


def remove_files():
    for filename in ["file1.txt", "file2.txt", "file3.txt"]:
        if os.path.exists(filename):
            os.remove(filename)


def minigit_run(*args):
    result = subprocess.run(
        ["C:\\Users\\danab\\Documents\\CourseWeb\\cpp2\\PetProjects\\Minigit\\main.exe", *args],
        capture_output=True,
        text=True
    )
    return result


class Initialization(unittest.TestCase):

    def setUp(self):
        remove_repository()

    def tearDown(self):
        remove_repository()

    def test_init_creates_repository(self):
        minigit_run("init")
        self.assertTrue(os.path.exists(".minigit"))

    def test_double_initialization(self):
        minigit_run("init")
        result = minigit_run("init")
        self.assertRegex(result.stdout, ".*Repository already initialized")

    def test_status_after_initialization(self):
        minigit_run("init")
        result = minigit_run("status")
        self.assertRegex(result.stdout, "On branch master")

    def test_log_after_initialization(self):
        minigit_run("init")
        result = minigit_run("log")
        self.assertEqual(result.stdout, "")


class Status(unittest.TestCase):

    def setUp(self):
        remove_repository()
        minigit_run("init")

    def tearDown(self):
        remove_files()
        remove_repository()

    def test_repo_not_initialized(self):
        remove_repository()
        result = minigit_run("status")
        self.assertRegex(result.stdout, "Error: Repository not initialized.")

    def test_untracked_files(self):
        f1 = open("file1.txt", "w")
        f1.close()
        result = minigit_run("status")
        self.assertRegex(result.stdout, "Untracked files:\n\tfile1.txt")

    def test_files_statuses(self):
        f1 = open("file1.txt", "w")
        f1.close()
        f2 = open("file2.txt", "w")
        f2.close()
        f3 = open("file3.txt", "w")
        f3.close()
        result = minigit_run("status")
        self.assertRegex(result.stdout, "Untracked files:\n\tfile1.txt\n\tfile2.txt\n\tfile3.txt")
        minigit_run("add", "file2.txt")
        result = minigit_run("status")
        self.assertRegex(result.stdout, "Changes to be committed:\n\tfile2.txt")
        self.assertRegex(result.stdout, "Untracked files:\n\tfile1.txt\n\tfile3.txt")
        minigit_run("add", "file3.txt")

        # Check that changes are detected, even after file is staged
        f3 = open("file3.txt", "w")
        f3.write("Some text")
        f3.close()
        result = minigit_run("status")
        self.assertRegex(result.stdout, "Changes to be committed:\n\tfile2.txt\n\tfile3.txt")
        self.assertRegex(result.stdout, "Changes not staged for commit:\n\tfile3.txt")
        self.assertRegex(result.stdout, "Untracked files:\n\tfile1.txt")

        # Check status after commit
        minigit_run("commit", "-m", "\"Added files file2.txt and file3.txt\"")
        result = minigit_run("status")
        self.assertNotRegex(result.stdout, "Changes to be committed:\n\tfile2.txt\n\tfile3.txt")
        self.assertRegex(result.stdout, "Changes not staged for commit:\n\tfile3.txt")
        self.assertRegex(result.stdout, "Untracked files:\n\tfile1.txt")
        minigit_run("add", "file1.txt", "file3.txt")
        minigit_run("commit", "-m", "\"Added file1.txt and modified file3.txt\"")
        result = minigit_run("status")
        self.assertNotRegex(result.stdout, "Changes to be committed")


class Staging(unittest.TestCase):

    def setUp(self):
        remove_repository()
        minigit_run("init")

    def tearDown(self):
        remove_files()
        remove_repository()

    def test_incorrect_usage(self):
        remove_repository()
        result = minigit_run("add")
        self.assertRegex(result.stdout, "Usage: minigit add <file1> <file2> <file3>")

    def test_repo_not_initialized(self):
        remove_repository()
        result = minigit_run("add", "file1.txt")
        self.assertRegex(result.stdout, "Error: Repository not initialized.")

    def test_add_nonexistent_file(self):
        result = minigit_run("add", "file1.txt")
        self.assertRegex(result.stdout, "ERROR: file file1.txt did not match any files")

    def test_json_file_and_blob_created(self):
        f1 = open("file1.txt", "w")
        f1.write("Some text")
        f1.close()
        minigit_run("add", "file1.txt")
        with open(".minigit/index.json", "r") as file:
            data = json.load(file)
        self.assertIn("file1.txt", data["tracked_files"])
        file_hash = data["tracked_files"]["file1.txt"]
        self.assertTrue(os.path.exists(".minigit\\objects\\blobs\\" + file_hash))
        f1_copy = open(".minigit\\objects\\blobs\\" + file_hash, "r")
        content = f1_copy.read()
        self.assertEqual(content, "Some text")
        f1_copy.close()


class Commit(unittest.TestCase):

    def setUp(self):
        remove_repository()
        minigit_run("init")

    def tearDown(self):
        remove_files()
        remove_repository()

    def test_incorrect_usage(self):
        remove_repository()
        result = minigit_run("commit")
        self.assertRegex(result.stdout, "Usage: minigit commit -m \"message\"")

    def test_repo_not_initialized(self):
        remove_repository()
        result = minigit_run("commit", "-m", "\"Commit message\"")
        self.assertRegex(result.stdout, "Error: Repository not initialized.")

    def test_nothing_to_commit(self):
        result = minigit_run("commit", "-m", "\"Commit message\"")
        self.assertRegex(result.stdout, "Nothing to commit")
        f1 = open("file1.txt", "w")
        f1.close()
        minigit_run("add", "file1.txt")
        result = minigit_run("commit", "-m", "\"Created file\"")
        self.assertRegex(result.stdout, "Committed:")
        self.assertRegex(result.stdout, "file1.txt")
        # try committing again
        result = minigit_run("commit", "-m", "\"Committing twice\"")
        self.assertRegex(result.stdout, "Nothing to commit")

    def test_commit_file_and_logs(self):
        f1 = open("file1.txt", "w")
        f1.close()
        f2 = open("file2.txt", "w")
        f2.close()
        f3 = open("file3.txt", "w")
        f3.close()
        minigit_run("add", "file1.txt", "file2.txt", "file3.txt")
        result = minigit_run("commit", "-m", "\"Created files\"")
        self.assertRegex(result.stdout, "Committed:")
        self.assertRegex(result.stdout, "file1.txt")
        self.assertRegex(result.stdout, "file2.txt")
        self.assertRegex(result.stdout, "file3.txt")

        # test commit info saved to json file
        # First get current branch
        with open(".minigit/HEAD", "r") as file:
            branch_name = file.read()
        # Now get head commit id
        with open(".minigit/refs/heads/" + branch_name, "r") as file:
            commit_id = file.read()
        # Check commit info file has been written
        self.assertTrue(os.path.exists(".minigit\\objects\\commits\\" + commit_id))
        # Check commit info
        with open(".minigit\\objects\\commits\\" + commit_id, "r") as file:
            commit_info = json.load(file)
        self.assertEqual(commit_info["message"], "\"Created files\"")
        self.assertEqual(commit_info["id"], commit_id)
        # Cross-check with index information
        with open(".minigit/index.json", "r") as file:
            index_data = json.load(file)
        for filename in index_data["tracked_files"]:
            self.assertEqual(commit_info["file_hashes"][filename], index_data["tracked_files"][filename])

        # Now check logs
        # First check HEAD log
        with open(".minigit/logs/HEAD", "r") as file:
            head_log_data = json.load(file)
        self.assertEqual(head_log_data["log"][-1]["new_commit_id"], commit_id)
        self.assertEqual(head_log_data["log"][-1]["message"], "\"Created files\"")
        # Check branch log
        with open(".minigit/logs/refs/heads/" + branch_name, "r") as file:
            branch_log_data = json.load(file)
        self.assertEqual(branch_log_data["log"][-1]["new_commit_id"], commit_id)
        self.assertEqual(branch_log_data["log"][-1]["message"], "\"Created files\"")

        # Now make another commit and check commit file and logs again
        f1 = open("file1.txt", "w")
        f1.write("Some text")
        f1.close()
        minigit_run("add", "file1.txt")
        result = minigit_run("commit", "-m", "\"Changed file1.txt\"")
        self.assertRegex(result.stdout, "Committed:")
        self.assertRegex(result.stdout, "file1.txt")
        with open(".minigit/refs/heads/" + branch_name, "r") as file:
            new_commit_id = file.read()
        # Check commit info file has been written
        self.assertTrue(os.path.exists(".minigit\\objects\\commits\\" + new_commit_id))
        # Check commit info
        with open(".minigit\\objects\\commits\\" + new_commit_id, "r") as file:
            new_commit_info = json.load(file)
        self.assertEqual(new_commit_info["message"], "\"Changed file1.txt\"")
        self.assertEqual(new_commit_info["id"], new_commit_id)
        # Now also check that the parent commit id is correct
        self.assertEqual(new_commit_info["parent_1_id"], commit_id)
        # Cross-check with index information
        with open(".minigit/index.json", "r") as file:
            index_data = json.load(file)
        # Although only file1.txt has changed, all three files hashes are in the commit
        for filename in ["file1.txt", "file2.txt", "file3.txt"]:
            self.assertEqual(new_commit_info["file_hashes"][filename], index_data["tracked_files"][filename])
        # Now check logs
        # First check HEAD log
        with open(".minigit/logs/HEAD", "r") as file:
            head_log_data = json.load(file)
        self.assertEqual(head_log_data["log"][-1]["new_commit_id"], new_commit_id)
        self.assertEqual(head_log_data["log"][-1]["message"], "\"Changed file1.txt\"")
        # Check branch log
        with open(".minigit/logs/refs/heads/" + branch_name, "r") as file:
            branch_log_data = json.load(file)
        self.assertEqual(branch_log_data["log"][-1]["new_commit_id"], new_commit_id)
        self.assertEqual(branch_log_data["log"][-1]["message"], "\"Changed file1.txt\"")
        self.assertEqual(branch_log_data["log"][-1]["old_commit_id"], commit_id)


class Log(unittest.TestCase):

    def setUp(self):
        remove_repository()
        minigit_run("init")

    def tearDown(self):
        remove_files()
        remove_repository()

    def test_repo_not_initialized(self):
        remove_repository()
        result = minigit_run("log")
        self.assertRegex(result.stdout, "Error: Repository not initialized.")

    def test_log(self):

        # Make a commit and check the right message is in the log
        f1 = open("file1.txt", "w")
        f1.write("Some text")
        f1.close()
        minigit_run("add", "file1.txt")
        minigit_run("commit", "-m", "\"Created file1.txt\"")
        result = minigit_run("log")
        self.assertRegex(result.stdout, "\"Created file1.txt\"")

        # Make another commit and check the log
        f1 = open("file1.txt", "w")
        f1.write("\nLine two")
        f1.close()
        minigit_run("add", "file1.txt")
        minigit_run("commit", "-m", "\"Added a line in file1.txt\"")
        result = minigit_run("log")
        # Check also if the log is in reverse chronological order (newest first)
        self.assertRegex(result.stdout,
                         "\"Added a line in file1.txt\"\n\ncommit.*\nAuthor:.*\nDate:.*\n\n\"Created file1.txt\"")


class Branch(unittest.TestCase):

    def setUp(self):
        remove_repository()
        minigit_run("init")

    def tearDown(self):
        remove_repository()

    def test_incorrect_usage(self):
        remove_repository()
        result = minigit_run("branch", "-b", "dev")
        self.assertRegex(result.stdout, "Usage: minigit branch <branch name> OR minigit branch")

    def test_repo_not_initialized(self):
        remove_repository()
        result = minigit_run("branch")
        self.assertRegex(result.stdout, "Error: Repository not initialized.")

    def test_create_branch_from_empty_master(self):
        result = minigit_run("branch", "dev_branch_1")
        self.assertRegex(result.stdout,
                         "ERROR: Cannot create new branch since there are no commits on the current branch")

    def test_create_and_list_branches(self):
        f1 = open("file1.txt", "w")
        f1.close()
        minigit_run("add", "file1.txt")
        minigit_run("commit", "-m", "Created file1.txt")
        # Now get the head commit id to cross-check it after creating new branch
        with open(".minigit/refs/heads/master", "r") as file:
            commit_id = file.read()

        minigit_run("branch", "dev_branch_1")
        minigit_run("branch", "dev_branch_2")
        # Check that the 'branch' command lists all branches
        result = minigit_run("branch")
        self.assertRegex(result.stdout,
                         "dev_branch_1\ndev_branch_2\nmaster")

        # The commit id of the commit made on master should be in the
        # logs, as well as the HEAD files of the new created branches
        with open(".minigit/refs/heads/dev_branch_1", "r") as file:
            dev_branch_1_head_id = file.read()
        self.assertEqual(dev_branch_1_head_id, commit_id)
        with open(".minigit/refs/heads/dev_branch_2", "r") as file:
            dev_branch_2_head_id = file.read()
        self.assertEqual(dev_branch_2_head_id, commit_id)
        # Check one of the log files too
        with open(".minigit/logs/refs/heads/dev_branch_1", "r") as file:
            branch_log_data = json.load(file)
        self.assertEqual(branch_log_data["log"][-1]["new_commit_id"], commit_id)
        self.assertEqual(branch_log_data["log"][-1]["message"], "Created file1.txt")

    def test_checkout_nonexistent_branch(self):
        result = minigit_run("checkout", "dev_branch_1")
        self.assertRegex(result.stdout, "ERROR: Branch does not exist.")

    def test_checkout_new_branch_with_staged_files(self):
        f1 = open("file1.txt", "w")
        f1.close()
        minigit_run("add", "file1.txt")
        minigit_run("commit", "-m", "Created file1.txt")
        f2 = open("file2.txt", "w")
        f2.close()
        minigit_run("add", "file2.txt")
        minigit_run("branch", "dev_branch_1")
        result = minigit_run("checkout", "dev_branch_1")
        self.assertRegex(result.stdout,
                         "ERROR: Cannot checkout another branch while "
                         "there are modified or staged \\(uncommitted\\) files.")

    def test_checkout_new_branch_with_modified_files(self):
        f1 = open("file1.txt", "w")
        f1.close()
        minigit_run("add", "file1.txt")
        minigit_run("commit", "-m", "Created file1.txt")
        f1 = open("file1.txt", "w")
        f1.write("Some text")
        f1.close()
        minigit_run("branch", "dev_branch_1")
        result = minigit_run("checkout", "dev_branch_1")
        self.assertRegex(result.stdout,
                         "ERROR: Cannot checkout another branch while "
                         "there are modified or staged \\(uncommitted\\) files.")

    def test_checkout_new_branch(self):
        f1 = open("file1.txt", "w")
        f1.close()
        minigit_run("add", "file1.txt")
        minigit_run("commit", "-m", "Created file1.txt")
        minigit_run("branch", "dev_branch_1")
        minigit_run("checkout", "dev_branch_1")
        # Check if current branch has changed
        with open(".minigit/HEAD", "r") as file:
            branch_name = file.read()
        self.assertEqual(branch_name, "dev_branch_1")

    def test_checkout_branch_ahead_of_master(self):
        # Check HEAD log changes when changing to a branch with different head
        f1 = open("file1.txt", "w")
        f1.close()
        minigit_run("add", "file1.txt")
        minigit_run("commit", "-m", "Created file1.txt")
        # Retrieve HEAD id to check later against new commit on new branch
        with open(".minigit/refs/heads/master", "r") as file:
            master_head_id = file.read()
        # Retrieve index file contents to see if it is restored after switching back to master
        with open(".minigit/index.json", "r") as file:
            master_index_data = json.load(file)
        minigit_run("branch", "dev_branch_1")
        minigit_run("checkout", "dev_branch_1")
        f1 = open("file1.txt", "w")
        f1.write("Added some text")
        f1.close()
        minigit_run("add", "file1.txt")
        minigit_run("commit", "-m", "Added text to file1.txt")
        # Check HEAD id for the new branch is different from master's HEAD id
        with open(".minigit/refs/heads/dev_branch_1", "r") as file:
            dev_branch_1_head_id = file.read()
        self.assertNotEqual(dev_branch_1_head_id, master_head_id)
        # New branch index data should now differ from master's
        with open(".minigit/index.json", "r") as file:
            dev_branch_1_index_data = json.load(file)
        self.assertNotEqual(dev_branch_1_index_data, master_index_data)
        # Now switch back to master
        minigit_run("checkout", "master")
        # HEAD should point to master again
        with open(".minigit/HEAD", "r") as file:
            self.assertEqual(file.read(), "master")
        # The index data should be restored to the master's
        with open(".minigit/index.json", "r") as file:
            index_data = json.load(file)
        self.assertEqual(index_data, master_index_data)
        # The working directory files should be restored to master's last commit
        f1 = open("file1.txt", "r")
        self.assertEqual(f1.read(), "")
        f1.close()
        # The change in HEAD should be logged
        with open(".minigit/logs/HEAD", "r") as file:
            head_log_data = json.load(file)
        self.assertEqual(head_log_data["log"][-1]["old_commit_id"], dev_branch_1_head_id)
        self.assertEqual(head_log_data["log"][-1]["new_commit_id"], master_head_id)
        self.assertEqual(head_log_data["log"][-1]["message"], "Switched to branch master")

        # Switch back to dev_branch_1
        minigit_run("checkout", "dev_branch_1")
        # HEAD should point to dev_branch_1 again
        with open(".minigit/HEAD", "r") as file:
            self.assertEqual(file.read(), "dev_branch_1")
        # The index data should be restored to the dev_branch_1's
        with open(".minigit/index.json", "r") as file:
            index_data = json.load(file)
        self.assertEqual(index_data, dev_branch_1_index_data)
        # The working directory files should be restored to master's last commit
        f1 = open("file1.txt", "r")
        self.assertEqual(f1.read(), "Added some text")
        f1.close()
        # The change in HEAD should be logged
        with open(".minigit/logs/HEAD", "r") as file:
            head_log_data = json.load(file)
        self.assertEqual(head_log_data["log"][-1]["old_commit_id"], master_head_id)
        self.assertEqual(head_log_data["log"][-1]["new_commit_id"], dev_branch_1_head_id)
        self.assertEqual(head_log_data["log"][-1]["message"], "Switched to branch dev_branch_1")


if __name__ == '__main__':
    unittest.main()
