#pragma once

#include "../store.h"
#include <string>
#include <filesystem>
#include <memory>

namespace backtrader {
namespace stores {

/**
 * VChartFile - Visual Chart binary file store implementation
 * 
 * Store provider for Visual Chart binary files.
 * Handles file-based data access for Visual Chart format.
 */
class VChartFile : public Store {
public:
    // Parameters structure
    struct Params {
        std::string path;  // Path to Visual Chart data directory
    };

    // Static factory method for singleton pattern
    static std::shared_ptr<VChartFile> getInstance(const Params& params = Params{});
    
    virtual ~VChartFile() = default;

    // Store interface implementation
    std::shared_ptr<DataSeries> getdata(const std::vector<std::any>& args = {}, 
                                       const std::map<std::string, std::any>& kwargs = {}) override;
    
    static std::shared_ptr<Broker> getbroker(const std::vector<std::any>& args = {}, 
                                            const std::map<std::string, std::any>& kwargs = {});

    // VChart specific methods
    std::string get_datapath() const;
    bool is_valid_path() const;
    
    // File operations
    std::vector<std::string> list_symbols() const;
    bool symbol_exists(const std::string& symbol) const;
    std::string get_symbol_path(const std::string& symbol) const;

private:
    VChartFile(const Params& params);
    
    // Singleton management
    static std::shared_ptr<VChartFile> instance_;
    static std::mutex instance_mutex_;
    
    // Parameters
    Params params_;
    
    // Data path
    std::string data_path_;
    
    // Visual Chart registry/directory constants
    static constexpr const char* VC_KEYNAME = "SOFTWARE\\VCG\\Visual Chart 6\\Config";
    static constexpr const char* VC_KEYVAL = "DocsDirectory";
    static constexpr std::array<const char*, 3> VC_DATADIR = {"Realserver", "Data", "01"};
    
    // Platform-specific path finding
    std::string find_vchart_path();
    
#ifdef _WIN32
    // Windows registry access
    std::string find_vchart_registry();
#endif
    
    // Path validation
    bool validate_path(const std::string& path);
    
    // Initialize data path
    void initialize_path();
};

} // namespace stores
} // namespace backtrader