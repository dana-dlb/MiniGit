#include <filesystem>
#include <string>

const std::string MINIGIT_FILES_PATH = ".minigit";
const std::filesystem::path MINIGIT_HEAD_PATH = std::filesystem::path(MINIGIT_FILES_PATH) / "HEAD";
const std::filesystem::path MINIGIT_MERGING_FLAG_PATH = std::filesystem::path(MINIGIT_FILES_PATH) / "MERGING";
const std::filesystem::path MINIGIT_MERGE_HEAD_PATH = std::filesystem::path(MINIGIT_FILES_PATH) / "MERGE_HEAD";
const std::filesystem::path MINIGIT_INDEX_PATH = std::filesystem::path(MINIGIT_FILES_PATH) / "index.json";
const std::filesystem::path MINIGIT_REFS_PATH = std::filesystem::path(MINIGIT_FILES_PATH) / "refs";
const std::filesystem::path MINIGIT_BRANCHES_PATH = std::filesystem::path(MINIGIT_FILES_PATH) / "refs" / "heads";
const std::filesystem::path MINIGIT_OBJECTS_PATH = std::filesystem::path(MINIGIT_FILES_PATH) / "objects";
const std::filesystem::path MINIGIT_COMMITS_PATH = std::filesystem::path(MINIGIT_FILES_PATH) / "objects" / "commits";
const std::filesystem::path MINIGIT_BLOBS_PATH = std::filesystem::path(MINIGIT_FILES_PATH) / "objects" / "blobs";
const std::filesystem::path MINIGIT_LOGS_PATH = std::filesystem::path(MINIGIT_FILES_PATH) / "logs";
const std::filesystem::path MINIGIT_HEAD_LOG_PATH = std::filesystem::path(MINIGIT_FILES_PATH) / "logs" / "HEAD";
const std::filesystem::path MINIGIT_LOG_REFS_PATH = std::filesystem::path(MINIGIT_FILES_PATH) / "logs" / "refs";
const std::filesystem::path MINIGIT_BRANCHES_LOG_PATH = std::filesystem::path(MINIGIT_FILES_PATH) / "logs" / "refs" / "heads";
const std::string MINIGIT_MASTER_BRANCH_NAME = "master";
const int MINIGIT_SHA_DIGEST_LENGTH = 20;

