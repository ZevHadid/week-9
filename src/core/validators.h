#pragma once
#include "student.h"

#include <string>

namespace core {

std::string normalizeGender(const std::string& raw);

bool validateGradeYear(const std::string& raw, std::string& err);
bool validateBirthDate(const std::string& raw, std::string& err);
bool validateFullName (const std::string& raw, std::string& err);
bool validateGrade    (double value, std::string& err);
bool validateStudent  (const Student& s, std::string& err);

bool parseGrade(const std::string& raw, double& out, std::string& err);

}