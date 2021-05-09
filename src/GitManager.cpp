#ifdef USE_LIBGIT2

#include "GitManager.h"
#include "FileFinder.h"
#include "json.hpp"

using JSON = nlohmann::json;

std::vector<std::u16string> GitManager::names = {
	AddComponent(u"GitFor1C", []() { return new GitManager; }),
};

GitManager::GitManager()
{
	git_libgit2_init();

	AddProperty(u"Head", u"Head", [&](VH var) { var = this->head(); });
	AddProperty(u"Remotes", u"Remotes", [&](VH var) { var = this->remoteList(); });
	AddProperty(u"Branches", u"Branches", [&](VH var) { var = this->branchList(); });
	AddProperty(u"Signature", u"Подпись", [&](VH var) { var = this->signature(); });

	AddProperty(u"Username", u"Логин", [&](VH var) { var = this->m_credential.username; }, [&](VH var) { this->m_credential.username = (std::string)var; });
	AddProperty(u"Password", u"Пароль", [&](VH var) { var = this->m_credential.password; }, [&](VH var) { this->m_credential.password = (std::string)var; });
	AddProperty(u"privkey", u"ПриватныйКлюч", [&](VH var) { var = this->m_credential.privkey; }, [&](VH var) { this->m_credential.privkey = (std::string)var; });
	AddProperty(u"pubkey", u"ПубличныйКлюч", [&](VH var) { var = this->m_credential.pubkey; }, [&](VH var) { this->m_credential.pubkey = (std::string)var; });

	AddProcedure(u"SetAuthor", u"SetAuthor", [&](VH name, VH email) { this->setAuthor(name, email); });
	AddProcedure(u"SetCommitter", u"SetCommitter", [&](VH name, VH email) { this->setCommitter(name, email); });

	AddFunction(u"Init", u"Init", [&](VH path) { this->result = this->init(path); });
	AddFunction(u"Open", u"Open", [&](VH path) { this->result = this->open(path); });
	AddFunction(u"Find", u"Find", [&](VH path) { this->result = this->find(path); });
	AddFunction(u"Clone", u"Clone", [&](VH url, VH path) { this->result = this->clone(url, path); });
	AddFunction(u"Close", u"Close", [&]() { this->result = this->close(); });
	AddFunction(u"Info", u"Info", [&](VH id) { this->result = this->info(id); });
	AddFunction(u"Diff", u"Diff", [&](VH p1, VH p2) { this->result = this->diff(p1, p2); });
	AddFunction(u"File", u"File", [&](VH path, VH full) { this->result = this->file(path, full); });
	AddFunction(u"Blob", u"Blob", [&](VH id, VH encoding) { this->blob(id, encoding); }, { { 1, (int64_t)0 } });
	AddFunction(u"Tree", u"Tree", [&](VH id) { this->result = this->tree(id); });
	AddFunction(u"Status", u"Status", [&]() { this->result = this->status(); });
	AddFunction(u"Commit", u"Commit", [&](VH msg) { this->result = this->commit(msg); });
	AddFunction(u"Checkout", u"Checkout", [&](VH name, VH create) { this->result = this->checkout(name, create); }, { {1, false} });
	AddFunction(u"Fetch", u"Fetch", [&](VH name, VH ref) { this->result = this->fetch(name, ref); }, { { 1, u"" } });
	AddFunction(u"Push", u"Push", [&](VH name, VH ref) { this->result = this->push(name, ref); }, { { 1, u"" } });
	AddFunction(u"Add", u"Add", [&](VH append, VH remove) { this->result = this->add(append, remove); }, { {1, u""} });
	AddFunction(u"Reset", u"Reset", [&](VH path) { this->result = this->reset(path); });
	AddFunction(u"Remove", u"Remove", [&](VH path) { this->result = this->remove(path); });
	AddFunction(u"Discard", u"Discard", [&](VH path) { this->result = this->discard(path); });
	AddFunction(u"History", u"History", [&](VH path) { this->result = this->history(path); }, { { 0, u"HEAD" } });
	AddFunction(u"IsBinary", u"IsBinary", [&](VH blob, VH encoding) { this->result = this->isBinary(blob, encoding); }, { {1, (int64_t)0} });
	AddFunction(u"GetEncoding", u"GetEncoding", [&](VH path) { this->result = this->getEncoding(path); });

	AddFunction(u"FindFiles", u"НайтиФайлы", [&](VH path, VH mask, VH text, VH ignore) {
		this->result = FileFinder(text, ignore).find(path, mask);
		}, { {4, true} });
}

#include <stdexcept>
#include <iostream>
#include <cstring>
#include <string>
#include <thread>

#ifdef _WINDOWS
#include <windows.h>
#include <conio.h>
#pragma comment(lib, "git2")
#pragma comment(lib, "crypt32")
#pragma comment(lib, "rpcrt4")
#pragma comment(lib, "winhttp")
#endif//_WINDOWS

#define CHECK_REPO() {if (m_repo == nullptr) return ::error(0);}

#define ASSERT(t) {if (t < 0) return ::error();}

template<class T, void destructor(T*)>
class AutoGit {
private:
	T* h = nullptr;
public:
	AutoGit() {}
	AutoGit(T* h) { this->h = h; }
	~AutoGit() { if (h) destructor(h); }
	operator T* () const { return h; }
	T** operator &() { return &h; }
	T* operator->() { return h; }
	operator bool() { return h; }
};

using GIT_branch_iterator = AutoGit<git_branch_iterator, git_branch_iterator_free>;
using GIT_status_list = AutoGit<git_status_list, git_status_list_free>;
using GIT_signature = AutoGit<git_signature, git_signature_free>;
using GIT_reference = AutoGit<git_reference, git_reference_free>;
using GIT_revwalk = AutoGit<git_revwalk, git_revwalk_free>;
using GIT_commit = AutoGit<git_commit, git_commit_free>;
using GIT_object = AutoGit<git_object, git_object_free>;
using GIT_remote = AutoGit<git_remote, git_remote_free>;
using GIT_index = AutoGit<git_index, git_index_free>;
using GIT_blob = AutoGit<git_blob, git_blob_free>;
using GIT_diff = AutoGit<git_diff, git_diff_free>;
using GIT_tree = AutoGit<git_tree, git_tree_free>;

GitManager::~GitManager()
{
	if (m_repo) git_repository_free(m_repo);
	if (m_committer) delete m_committer;
	if (m_author) delete m_author;
	git_libgit2_shutdown();
}

static std::string success(const JSON& result)
{
	JSON json;
	json["result"] = result;
	json["success"] = true;
	return json.dump();
}

static std::string error()
{
	const git_error* e = git_error_last();
	JSON json, j;
	j["code"] = e->klass;
	j["message"] = e->message;
	json["error"] = j;
	json["success"] = false;
	return json.dump();
}

static std::string error(int code)
{
	JSON json, j;
	j["code"] = code;
	j["message"] = "Repo is null";
	json["error"] = j;
	json["success"] = false;
	return json.dump();
}

static std::string error(const std::string& msg)
{
	JSON json, j;
	j["code"] = -1;
	j["message"] = msg;
	json["error"] = j;
	json["success"] = false;
	return json.dump();
}

std::string GitManager::init(const std::string& path)
{
	if (m_repo) git_repository_free(m_repo);
	m_repo = nullptr;
	std::string p = path;
	ASSERT(git_repository_init(&m_repo, p.c_str(), false));
	return success(true);
}

std::string GitManager::clone(const std::string& url, const std::string& path)
{
	if (m_repo) git_repository_free(m_repo);
	m_repo = nullptr;
	ASSERT(git_clone(&m_repo, url.c_str(), path.c_str(), nullptr));
	return success(true);
}

std::string GitManager::open(const std::string& path)
{
	if (m_repo) git_repository_free(m_repo);
	m_repo = nullptr;
	ASSERT(git_repository_open(&m_repo, path.c_str()));
	return success(true);
}

std::string GitManager::close()
{
	if (m_repo) git_repository_free(m_repo);
	m_repo = nullptr;
	return success(true);
}

std::string GitManager::find(const std::string& path)
{
	std::string res;
	git_buf buffer = { 0 };
	ASSERT(git_repository_discover(&buffer, path.c_str(), 0, nullptr));
	if (buffer.ptr) res = buffer.ptr;
	git_buf_free(&buffer);
	return success(res);
}

static std::string status2str(git_status_t status) {
	switch (status) {
	case GIT_STATUS_CURRENT: return "CURRENT";
	case GIT_STATUS_INDEX_NEW: return "INDEX_NEW";
	case GIT_STATUS_INDEX_MODIFIED: return "INDEX_MODIFIED";
	case GIT_STATUS_INDEX_DELETED: return "INDEX_DELETED";
	case GIT_STATUS_INDEX_RENAMED: return "INDEX_RENAMED";
	case GIT_STATUS_INDEX_TYPECHANGE: return "INDEX_TYPECHANGE";
	case GIT_STATUS_WT_NEW: return "WT_NEW";
	case GIT_STATUS_WT_MODIFIED: return "WT_MODIFIED";
	case GIT_STATUS_WT_DELETED: return "WT_DELETED";
	case GIT_STATUS_WT_TYPECHANGE: return "WT_TYPECHANGE";
	case GIT_STATUS_WT_RENAMED: return "WT_RENAMED";
	case GIT_STATUS_WT_UNREADABLE: return "WT_UNREADABLE";
	case GIT_STATUS_IGNORED: return "IGNORED";
	case GIT_STATUS_CONFLICTED: return "CONFLICTED";
	default: return {};
	}
}

static std::string delta2str(git_delta_t status) {
	switch (status) {
	case GIT_DELTA_UNMODIFIED: return "UNMODIFIED";
	case GIT_DELTA_ADDED: return "ADDED";
	case GIT_DELTA_DELETED: return "DELETED";
	case GIT_DELTA_MODIFIED: return "MODIFIED";
	case GIT_DELTA_RENAMED: return "RENAMED";
	case GIT_DELTA_COPIED: return "COPIED";
	case GIT_DELTA_IGNORED: return "IGNORED";
	case GIT_DELTA_UNTRACKED: return "UNTRACKED";
	case GIT_DELTA_TYPECHANGE: return "TYPECHANGE";
	case GIT_DELTA_UNREADABLE: return "UNREADABLE";
	case GIT_DELTA_CONFLICTED: return "CONFLICTED";
	default:  return "UNMODIFIED";
	}
}

static std::string diff2str(git_diff_flag_t flag) {
	switch (flag) {
	case GIT_DIFF_FLAG_BINARY: return "BINARY";
	case GIT_DIFF_FLAG_NOT_BINARY: return "TEXT";
	case GIT_DIFF_FLAG_VALID_ID: return "VALID";
	case GIT_DIFF_FLAG_EXISTS: return "EXISTS";
	}
}

static JSON flags2json(unsigned int status_flags) {
	JSON json;
	for (unsigned int i = 0; i < 16; i++) {
		git_status_t status = git_status_t(1u << i);
		if (status & status_flags) {
			json.push_back(status2str(status));
		}
	}
	return json;
}

static JSON diff2json(unsigned int status_flags) {
	JSON json;
	for (unsigned int i = 0; i < 4; i++) {
		git_diff_flag_t flag = git_diff_flag_t(1u << i);
		if (flag & status_flags) {
			json.push_back(diff2str(flag));
		}
	}
	return json;
}

static std::string oid2str(const git_oid* id)
{
	if (git_oid_is_zero(id)) return {};
	const size_t size = GIT_OID_HEXSZ + 1;
	char buf[size];
	git_oid_tostr(buf, size, id);
	return buf;
}

int status_cb(const char* path, unsigned int status_flags, void* payload)
{
	JSON j;
	j["filepath"] = path;
	j["statuses"] = flags2json(status_flags);
	((JSON*)payload)->push_back(j);
	return 0;
}

JSON delta2json(const git_diff_delta* delta, bool index = false)
{
	JSON j;
	j["flag"] = delta->status;
	j["status"] = delta2str(delta->status);
	j["similarity"] = delta->similarity;
	j["nfiles"] = delta->nfiles;
	j["old_id"] = oid2str(&delta->old_file.id);
	j["old_name"] = delta->old_file.path;
	j["old_size"] = delta->old_file.size;
	j["old_flags"] = delta->old_file.flags;
	if (index) j["new_id"] = oid2str(&delta->new_file.id);
	j["new_name"] = delta->new_file.path;
	j["new_size"] = delta->new_file.size;
	j["new_flags"] = delta->new_file.flags;
	return j;
}

JSON commit2json(const git_commit* commit)
{
	JSON j;
	const git_oid* tree_id = git_commit_tree_id(commit);
	const git_signature* author = git_commit_author(commit);
	const git_signature* committer = git_commit_committer(commit);
	j["id"] = oid2str(tree_id);
	j["authorName"] = author->name;
	j["authorEmail"] = author->email;
	j["committerName"] = committer->name;
	j["committerEmail"] = committer->email;
	j["message"] = git_commit_message(commit);
	j["time"] = git_commit_time(commit);
	return j;
}

std::string GitManager::status()
{
	CHECK_REPO();
	JSON json, jIndex, jWork;
	GIT_status_list statuses = NULL;
	git_status_options opts = GIT_STATUS_OPTIONS_INIT;
	opts.flags = GIT_STATUS_OPT_DEFAULTS;
	ASSERT(git_status_list_new(&statuses, m_repo, &opts));
	size_t count = git_status_list_entrycount(statuses);
	for (size_t i = 0; i < count; ++i) {
		const git_status_entry* entry = git_status_byindex(statuses, i);
		if (entry->head_to_index) jIndex.push_back(delta2json(entry->head_to_index, true));
		if (entry->index_to_workdir) jWork.push_back(delta2json(entry->index_to_workdir));
	}
	if (jIndex.is_array()) json["index"] = jIndex;
	if (jWork.is_array()) json["work"] = jWork;
	return success(json);
}

void GitManager::setAuthor(const std::string& name, const std::string& email)
{
	if (m_author) delete m_author;
	m_author = new Signature(name, email);
}

void GitManager::setCommitter(const std::string& name, const std::string& email)
{
	if (m_committer) delete m_committer;
	m_committer = new Signature(name, email);
}

std::string GitManager::branchList()
{
	CHECK_REPO();
	GIT_branch_iterator iterator;
	ASSERT(git_branch_iterator_new(&iterator, m_repo, GIT_BRANCH_LOCAL));
	git_reference* ref;
	git_branch_t type;
	JSON json;
	while (git_branch_next(&ref, &type, iterator) == 0) {
		const char* name;
		GIT_reference reference(ref);
		if (git_branch_name(&name, reference) == 0) {
			if (name) json.push_back(std::string(name));
		}
	}
	return success(json);
}

std::string GitManager::remoteList()
{
	CHECK_REPO();
	git_strarray strarray;
	ASSERT(git_remote_list(&strarray, m_repo));
	JSON json;
	for (size_t i = 0; i < strarray.count; i++) {
		GIT_remote remote;
		ASSERT(git_remote_lookup(&remote, m_repo, strarray.strings[i]));
		JSON j;
		j["name"] = strarray.strings[i];
		j["url"] = git_remote_url(remote);
		json.push_back(j);
	}
	git_strarray_free(&strarray);
	return success(json);
}

std::string GitManager::signature()
{
	CHECK_REPO();
	git_signature* sig = nullptr;
	ASSERT(git_signature_default(&sig, m_repo));
	JSON j;
	j["name"] = sig->name;
	j["email"] = sig->email;
	return success(j);
}

std::string GitManager::commit(const std::string& msg)
{
	CHECK_REPO();
	GIT_signature sig;
	GIT_signature author;
	GIT_signature committer;
	if (m_author) m_author->now(&author);
	if (m_committer) m_committer->now(&committer);
	if (m_author == nullptr || m_committer == nullptr) {
		ASSERT(git_signature_default(&sig, m_repo));
	}

	int ok = 0;
	GIT_index index;
	GIT_tree tree;
	git_oid tree_id, commit_id;
	git_object* head_commit = nullptr;
	git_revparse_single(&head_commit, m_repo, "HEAD^{commit}");
	GIT_commit commit = (git_commit*)head_commit;
	size_t head_count = head_commit ? 1 : 0;
	ASSERT(git_repository_index(&index, m_repo));
	ASSERT(git_index_write_tree(&tree_id, index));
	ASSERT(git_tree_lookup(&tree, m_repo, &tree_id));

	ASSERT(git_commit_create_v(
		&commit_id,
		m_repo,
		"HEAD",
		author ? author : sig,
		committer ? committer : sig,
		NULL,
		msg.c_str(),
		tree,
		head_count,
		head_commit
	));
	return success(true);
}

static JSON parse_file_list(std::string filelist)
{
	try {
		return JSON::parse(filelist);
	}
	catch (JSON::parse_error e) {
		JSON json;
		json.push_back(filelist);
		return json;
	}
}

std::string GitManager::add(const std::string& append, const std::string& remove)
{
	CHECK_REPO();
	GIT_index index;
	ASSERT(git_repository_index(&index, m_repo));
	JSON json_add = parse_file_list(append);
	if (json_add.is_array()) {
		for (auto element : json_add) {
			std::string path = element;
			ASSERT(git_index_add_bypath(index, path.c_str()));
		}
	}
	JSON json_del = parse_file_list(remove);
	if (json_del.is_array()) {
		for (auto element : json_del) {
			std::string path = element;
			ASSERT(git_index_remove_bypath(index, path.c_str()));
		}
	}
	ASSERT(git_index_write(index));
	return success(true);
}

std::string GitManager::reset(const std::string& filelist)
{
	CHECK_REPO();
	GIT_object obj = NULL;
	ASSERT(git_revparse_single(&obj, m_repo, "HEAD^{commit}"));
	JSON json = parse_file_list(filelist);
	if (json.is_array()) {
		for (auto element : json) {
			std::string path = element;
			const char* paths[] = { path.c_str() };
			const git_strarray strarray = { (char**)paths, 1 };
			ASSERT(git_reset_default(m_repo, obj, &strarray));
		}
	}
	return success(true);
}

std::string GitManager::discard(const std::string& filelist)
{
	CHECK_REPO();
	git_checkout_options options;
	ASSERT(git_checkout_options_init(&options, GIT_CHECKOUT_OPTIONS_VERSION));
	options.checkout_strategy = GIT_CHECKOUT_FORCE;
	options.paths.count = 1;
	JSON json = parse_file_list(filelist);
	if (json.is_array()) {
		for (auto element : json) {
			std::string path = element;
			const char* paths[] = { path.c_str() };
			options.paths.strings = (char**)paths;
			ASSERT(git_checkout_head(m_repo, &options));
		}
	}
	return success(true);
}

std::string GitManager::remove(const std::string& filelist)
{
	CHECK_REPO();
	GIT_index index;
	ASSERT(git_repository_index(&index, m_repo));
	JSON json = parse_file_list(filelist);
	if (json.is_array()) {
		for (auto element : json) {
			std::string path = element;
			ASSERT(git_index_remove_bypath(index, path.c_str()));
		}
	}
	ASSERT(git_index_write(index));
	return success(true);
}

std::string GitManager::info(const std::string& spec)
{
	CHECK_REPO();
	GIT_object head_commit;
	ASSERT(git_revparse_single(&head_commit, m_repo, spec.c_str()));
	return success(commit2json((git_commit*)(git_object*)head_commit));
}

std::string GitManager::head()
{
	CHECK_REPO();
	GIT_reference head;
	ASSERT(git_repository_head(&head, m_repo));
	std::string result;
	const char* name = git_reference_symbolic_target(head);
	if (name == nullptr) name = git_reference_name(head);
	if (name) result = name;
	return success(result);
}

std::string GitManager::history(const std::string& spec)
{
	CHECK_REPO();

	git_oid oid;
	GIT_revwalk walker;
	git_revwalk_new(&walker, m_repo);
	git_revwalk_sorting(walker, GIT_SORT_TIME);
	git_revwalk_push_head(walker);

	JSON json;
	while (git_revwalk_next(&oid, walker) == 0) {
		GIT_commit commit;
		ASSERT(git_commit_lookup(&commit, m_repo, &oid));
		json.push_back(commit2json(commit));
	}
	return success(json);
}

std::string type2str(git_object_t type) {
	switch (type) {
	case GIT_OBJECT_TREE: return "tree";
	case GIT_OBJECT_BLOB: return "blob";
	default: return {};
	}
}

int tree_walk_cb(const char* root, const git_tree_entry* entry, void* payload)
{
	JSON j;
	git_object_t type = git_tree_entry_type(entry);
	j["id"] = oid2str(git_tree_entry_id(entry));
	j["name"] = git_tree_entry_name(entry);
	j["type"] = type2str(type);
	j["root"] = root;
	((JSON*)payload)->push_back(j);
	return 0;
}

std::string GitManager::tree(const std::string& id)
{
	CHECK_REPO();

	git_object* obj = NULL;
	ASSERT(git_revparse_single(&obj, m_repo, "HEAD^{tree}"));
	GIT_tree tree = (git_tree*)obj;

	JSON json;
	ASSERT(git_tree_walk(tree, GIT_TREEWALK_PRE, tree_walk_cb, &json));
	return success(json);
}


int diff_file_cb(const git_diff_delta* delta, float progress, void* payload)
{
	((JSON*)payload)->push_back(delta2json(delta));
	return 0;
}

int diff_hunk_cb(const git_diff_delta* delta, const git_diff_hunk* hunk, void* payload)
{
	JSON j, & json = *(JSON*)payload;
	j["old_start"] = hunk->old_start;
	j["old_lines"] = hunk->old_lines;
	j["new_start"] = hunk->new_start;
	j["new_lines"] = hunk->new_lines;
	j["header_len"] = hunk->header_len;
	j["header"] = hunk->header;
	json["hunk"].push_back(j);
	return 0;
}

int diff_line_cb(const git_diff_delta* delta, const git_diff_hunk* hunk, const git_diff_line* line, void* payload)
{
	JSON j, &json = *(JSON*)payload;
	j["origin"] = std::string(1, line->origin);
	j["old_lineno"] = line->old_lineno;
	j["new_lineno"] = line->new_lineno;
	j["num_lines"] = line->new_lineno;
	j["content_len"] = line->content_len;
	j["content_offset"] = line->content_offset;
	json["line"].push_back(j);
	return 0;
}

std::string GitManager::diff(VH& old_data, VH& new_data)
{
	JSON json;
	json["hunk"] = JSON();
	json["line"] = JSON();
	ASSERT(git_diff_buffers(
		old_data.data(), old_data.size(), nullptr,
		new_data.data(), new_data.size(), nullptr,
		nullptr, nullptr, nullptr, diff_hunk_cb, diff_line_cb, &json));
	return json.dump();
}

std::string GitManager::diff_dir(const std::u16string& s1, const std::u16string& s2)
{
	CHECK_REPO();
	GIT_diff diff = NULL;
	if ((s1 == u"INDEX" && s2 == u"WORK") || (s2 == u"INDEX" && s1 == u"WORK")) {
		ASSERT(git_diff_index_to_workdir(&diff, m_repo, NULL, NULL));
	}
	else if ((s1 == u"HEAD" && s2 == u"INDEX") || (s2 == u"HEAD" && s1 == u"INDEX")) {
		GIT_object obj = NULL;
		ASSERT(git_revparse_single(&obj, m_repo, "HEAD^{tree}"));
		GIT_tree tree = NULL;
		ASSERT(git_tree_lookup(&tree, m_repo, git_object_id(obj)));
		ASSERT(git_diff_tree_to_index(&diff, m_repo, tree, NULL, NULL));
	}
	else if ((s1 == u"HEAD" && s2 == u"WORK") || (s2 == u"HEAD" && s1 == u"WORK")) {
		GIT_object obj = NULL;
		ASSERT(git_revparse_single(&obj, m_repo, "HEAD^{tree}"));
		GIT_tree tree = NULL;
		ASSERT(git_tree_lookup(&tree, m_repo, git_object_id(obj)));
		ASSERT(git_diff_tree_to_workdir_with_index(&diff, m_repo, tree, NULL));
	}
	JSON json;
	if (diff) {
		git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;
		opts.flags = GIT_DIFF_FIND_RENAMES | GIT_DIFF_FIND_COPIES | GIT_DIFF_FIND_FOR_UNTRACKED;
		ASSERT(git_diff_find_similar(diff, &opts));
		ASSERT(git_diff_foreach(diff, diff_file_cb, NULL, NULL, NULL, &json));
	}
	return success(json);
}

std::string GitManager::file(const std::string& path, bool full)
{
	if (m_repo == nullptr) return {};
	git_oid oid;
	int ok = full
		? git_blob_create_fromdisk(&oid, m_repo, path.c_str())
		: git_blob_create_from_workdir(&oid, m_repo, path.c_str());
	if (ok < 0) return {};
	return oid2str(&oid);
}

namespace GIT {

	typedef enum {
		GIT_BOM_NONE = 0,
		GIT_BOM_UTF8 = 1,
		GIT_BOM_UTF16_LE = 2,
		GIT_BOM_UTF16_BE = 3,
		GIT_BOM_UTF32_LE = 4,
		GIT_BOM_UTF32_BE = 5
	} git_bom_t;

	static int git_buf_text_detect_bom(git_bom_t* bom, const git_buf* buf)
	{
		const char* ptr;
		size_t len;

		*bom = GIT_BOM_NONE;
		/* need at least 2 bytes to look for any BOM */
		if (buf->size < 2)
			return 0;

		ptr = buf->ptr;
		len = buf->size;

		switch (*ptr++) {
		case 0:
			if (len >= 4 && ptr[0] == 0 && ptr[1] == '\xFE' && ptr[2] == '\xFF') {
				*bom = GIT_BOM_UTF32_BE;
				return 4;
			}
			break;
		case '\xEF':
			if (len >= 3 && ptr[0] == '\xBB' && ptr[1] == '\xBF') {
				*bom = GIT_BOM_UTF8;
				return 3;
			}
			break;
		case '\xFE':
			if (*ptr == '\xFF') {
				*bom = GIT_BOM_UTF16_BE;
				return 2;
			}
			break;
		case '\xFF':
			if (*ptr != '\xFE')
				break;
			if (len >= 4 && ptr[1] == 0 && ptr[2] == 0) {
				*bom = GIT_BOM_UTF32_LE;
				return 4;
			}
			else {
				*bom = GIT_BOM_UTF16_LE;
				return 2;
			}
			break;
		default:
			break;
		}

		return 0;
	}
}

void GitManager::blob(VH id, VH encoding)
{
	if (m_repo == nullptr) { result = ::error(0); return; };

	git_oid oid;
	int ok = git_oid_fromstr(&oid, std::string(id).c_str());
	if (ok < 0) { result = ::error(); return; }

	GIT_blob blob = NULL;
	ok = git_blob_lookup(&blob, m_repo, &oid);
	if (ok < 0) { result = ::error(); return; }

	git_off_t rawsize = git_blob_rawsize(blob);
	const void* rawcontent = git_blob_rawcontent(blob);
	if (rawsize <= 0) return;

	const git_buf buf = GIT_BUF_INIT_CONST(rawcontent, rawsize);
	result.AllocMemory((unsigned long)rawsize);
	memcpy((void*)result.data(), rawcontent, rawsize);

	GIT::git_bom_t bom;
	bool binary = git_buf_is_binary(&buf);
	if (!binary) GIT::git_buf_text_detect_bom(&bom, &buf);
	encoding = int64_t(binary ? -1 : bom);
}

bool GitManager::isBinary(VH blob, VH encoding)
{
	const git_buf buf = GIT_BUF_INIT_CONST(blob.data(), blob.size());
	GIT::git_bom_t bom;
	GIT::git_buf_text_detect_bom(&bom, &buf);
	bool binary = git_buf_is_binary(&buf);
	encoding = int64_t(binary ? -1 : bom);
	return binary;
}

int64_t GitManager::getEncoding(VH blob)
{
	const git_buf buf = GIT_BUF_INIT_CONST(blob.data(), blob.size());
	GIT::git_bom_t bom;
	GIT::git_buf_text_detect_bom(&bom, &buf);
	return bom;
}

std::string GitManager::checkout(const std::string& name, bool create)
{
	if (create) {
		git_object* head;
		const char* spec = "HEAD^{commit}";
		ASSERT(git_revparse_single(&head, m_repo, spec));
		GIT_commit commit = (git_commit*)head;
		GIT_reference reference;
		ASSERT(git_branch_create(&reference, m_repo, name.c_str(), commit, 0));
	}
	GIT_object treeish = NULL;
	git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
	opts.checkout_strategy = GIT_CHECKOUT_SAFE;
	ASSERT(git_revparse_single(&treeish, m_repo, name.c_str()));
	ASSERT(git_checkout_tree(m_repo, treeish, &opts));
	const std::string ref = "refs/heads/" + name;
	ASSERT(git_repository_set_head(m_repo, ref.c_str()));
	return success(true);
}

int credential_cb(git_cred** out, const char* url, const char* username_from_url, unsigned int allowed_types, void* payload)
{
	int error = 1;
	auto credential = (GitCredential*)payload;
	std::string username = credential->username;
	if (username_from_url) username = username_from_url;
	if (allowed_types & GIT_CREDENTIAL_SSH_KEY) {
		error = git_credential_ssh_key_new(out,
			username.c_str(),
			credential->pubkey.c_str(),
			credential->privkey.c_str(),
			credential->password.c_str()
		);
	}
	else if (allowed_types & GIT_CREDENTIAL_USERPASS_PLAINTEXT) {
		error = git_credential_userpass_plaintext_new(out,
			username.c_str(),
			credential->password.c_str()
		);
	}
	else if (allowed_types & GIT_CREDENTIAL_USERNAME) {
		error = git_credential_username_new(out, username.c_str());
	}
	return error;
}

std::string GitManager::fetch(const std::string& name, const std::string& ref)
{
	GIT_remote remote;
	git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
	git_fetch_options options = GIT_FETCH_OPTIONS_INIT;
	options.callbacks.credentials = credential_cb;
	options.callbacks.payload = &m_credential;
	ASSERT(git_remote_lookup(&remote, m_repo, name.c_str()));
	if (ref.empty()) {
		ASSERT(git_remote_fetch(remote, nullptr, &options, nullptr));
	}
	else {
		const char* paths[] = { ref.c_str() };
		const git_strarray strarray = { (char**)paths, 1 };
		ASSERT(git_remote_fetch(remote, &strarray, &options, nullptr));
	}
	return success(true);
}

std::string GitManager::push(const std::string& name, const std::string& ref)
{
	GIT_remote remote;
	git_push_options options;
	git_push_options_init(&options, GIT_PUSH_OPTIONS_VERSION);
	options.callbacks.credentials = credential_cb;
	options.callbacks.payload = &m_credential;
	ASSERT(git_remote_lookup(&remote, m_repo, name.c_str()));
	if (ref.empty()) {
		ASSERT(git_remote_push(remote, nullptr, &options));
	}
	else {
		const char* paths[] = { ref.c_str() };
		const git_strarray strarray = { (char**)paths, 1 };
		ASSERT(git_remote_push(remote, &strarray, &options));
	}
	return success(true);
}

std::string GitManager::compare(const std::string& name, const std::string& ref)
{
	GIT_revwalk walker;
	git_revwalk_new(&walker, m_repo);
	git_revwalk_push_ref(walker, "refs/remotes/origin/master");
	git_revwalk_hide_ref(walker, "refs/heads/master");

	git_oid id;
	long count = 0;
	while (!git_revwalk_next(&id, walker)) count++;
	return success(count);
}

#endif //USE_LIBGIT2