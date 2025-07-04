#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace backtrader {

class BacktraderError : public std::runtime_error {
public:
    BacktraderError(const std::string& message) : std::runtime_error(message) {}
    virtual ~BacktraderError() = default;
};

class StrategySkipError : public BacktraderError {
public:
    StrategySkipError(const std::string& message = "Strategy skip requested") 
        : BacktraderError(message) {}
    virtual ~StrategySkipError() = default;
};

class ModuleImportError : public BacktraderError {
public:
    ModuleImportError(const std::string& message, const std::vector<std::string>& args = {})
        : BacktraderError(message), args_(args) {}
    virtual ~ModuleImportError() = default;
    
    const std::vector<std::string>& get_args() const { return args_; }

private:
    std::vector<std::string> args_;
};

class FromModuleImportError : public ModuleImportError {
public:
    FromModuleImportError(const std::string& message, const std::vector<std::string>& args = {})
        : ModuleImportError(message, args) {}
    virtual ~FromModuleImportError() = default;
};

} // namespace backtrader