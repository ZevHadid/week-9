// app_core.h
#pragma once
#include "helper_functions.h"
#include <string>

namespace core {

enum class FileFormat { Csv, Xlsx };

// The currently "bound" file. Empty path = no file, no persistence.
struct Session {
    std::string path;
    FileFormat  format = FileFormat::Csv;
    bool        bound  = false;
    bool        dirty  = false;
};

extern Session session;

// --- format helpers ---
bool formatFromPath(const std::string& path, FileFormat& out);

// --- session lifecycle ---
// Load an existing file (validates the format/structure).
bool openSession(const std::string& path, std::string& err);

// Create a new empty session bound to `path`. If the file exists and
// overwrite is false, opens it instead (safer than clobbering).
bool newSession(const std::string& path, bool overwrite, std::string& err);

// Write the in-memory students back to the bound file.
bool saveSession(std::string& err);

// Drop students and unbind.
void closeSession();

// --- validation ---
bool parseScore(const std::string& raw, double& out, std::string& err);
bool validateFile(const std::string& path, std::string& err);
std::string normalizeGender(const std::string& raw);

// --- CRUD (each marks session dirty on success; indices are 0-based) ---
bool addStudent(const std::string& name, const std::string& gender,
                double score, std::string& err);
bool modifyStudent(size_t index, const std::string& name,
                   const std::string& gender, double score, std::string& err);
bool removeStudent(size_t index, std::string& err);

} // namespace core