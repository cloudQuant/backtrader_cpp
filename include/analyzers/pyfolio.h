#pragma once

#include "../analyzer.h"
#include "timereturn.h"
#include "positions.h"
#include "transactions.h"
#include "leverage.h"
#include <chrono>
#include <memory>
#include <vector>
#include <map>
#include <string>

namespace backtrader {
namespace analyzers {

/**
 * PyFolio analyzer - transforms backtrader data into pyfolio-compatible format
 * 
 * This analyzer serves as a composite wrapper that combines multiple child analyzers
 * to collect and transform data for compatibility with pyfolio (a Python library
 * for performance and risk analysis of financial portfolios).
 * 
 * The analyzer integrates four key components:
 * 1. TimeReturn: Portfolio returns over time
 * 2. PositionsValue: Position values for each asset
 * 3. Transactions: All trades/transactions
 * 4. GrossLeverage: Portfolio leverage calculations
 * 
 * The data is transformed into a format suitable for quantitative analysis
 * and can be exported to formats compatible with external analysis tools.
 * 
 * Parameters:
 *   - timeframe: Timeframe for analysis (default: Days)
 *   - compression: Compression factor (default: 1)
 * 
 * Features:
 *   - Composite analyzer pattern
 *   - Multi-analyzer data collection
 *   - Data transformation for external tools
 *   - Timezone-aware datetime handling
 *   - CSV export functionality
 * 
 * get_analysis():
 *   - Returns combined analysis from all child analyzers
 *   - Includes returns, positions, transactions, and leverage data
 * 
 * get_pf_items():
 *   - Returns data in pyfolio-compatible format
 *   - Structured for integration with quantitative analysis tools
 */
class PyFolio : public Analyzer {
public:
    // Parameters structure
    struct Params {
        TimeFrame::Value timeframe = TimeFrame::Days;  // Default timeframe
        int compression = 1;                           // Default compression
    };
    
    PyFolio(const Params& params = Params{});
    virtual ~PyFolio() = default;
    
    // Override analyzer methods
    void start() override;
    void stop() override;
    AnalysisResult get_analysis() const override;
    
    // PyFolio-specific methods
    struct PyFolioItems {
        // Returns data structure
        struct ReturnsData {
            std::vector<std::chrono::system_clock::time_point> dates;
            std::vector<double> values;
        } returns;
        
        // Positions data structure
        struct PositionsData {
            std::vector<std::chrono::system_clock::time_point> dates;
            std::vector<std::string> headers;
            std::vector<std::vector<double>> values;  // Matrix of position values
        } positions;
        
        // Transactions data structure
        struct TransactionsData {
            std::vector<std::chrono::system_clock::time_point> dates;
            std::vector<std::string> headers;
            std::vector<std::vector<double>> values;  // Matrix of transaction data
        } transactions;
        
        // Gross leverage data structure
        struct GrossLevData {
            std::vector<std::chrono::system_clock::time_point> dates;
            std::vector<double> values;
        } gross_lev;
    };
    
    // Get data in pyfolio-compatible format
    PyFolioItems get_pf_items() const;
    
    // Export methods
    void export_returns_csv(const std::string& filename) const;
    void export_positions_csv(const std::string& filename) const;
    void export_transactions_csv(const std::string& filename) const;
    void export_gross_lev_csv(const std::string& filename) const;
    void export_all_csv(const std::string& base_filename) const;
    
    // Access individual child analyzers
    std::shared_ptr<TimeReturn> get_returns_analyzer() const { return _returns; }
    std::shared_ptr<PositionsValue> get_positions_analyzer() const { return _positions; }
    std::shared_ptr<Transactions> get_transactions_analyzer() const { return _transactions; }
    std::shared_ptr<GrossLeverage> get_gross_lev_analyzer() const { return _gross_lev; }
    
    // Statistics and summary methods
    struct SummaryStatistics {
        double total_return;
        double annualized_return;
        double volatility;
        double sharpe_ratio;
        double max_drawdown;
        double calmar_ratio;
        size_t num_trades;
        double avg_trade_size;
        double hit_ratio;
        std::chrono::system_clock::time_point start_date;
        std::chrono::system_clock::time_point end_date;
        std::chrono::seconds duration;
    };
    
    SummaryStatistics get_summary_statistics() const;
    
    // Validation methods
    bool validate_data_consistency() const;
    std::vector<std::string> get_data_quality_issues() const;
    
protected:
    Params p;  // Parameters
    
    // Child analyzers
    std::shared_ptr<TimeReturn> _returns;
    std::shared_ptr<PositionsValue> _positions;
    std::shared_ptr<Transactions> _transactions;
    std::shared_ptr<GrossLeverage> _gross_lev;
    
    // Results storage
    AnalysisResult combined_results;
    
private:
    // Initialization helpers
    void initialize_child_analyzers();
    void setup_analyzer_relationships();
    
    // Data collection helpers
    void collect_returns_data();
    void collect_positions_data();
    void collect_transactions_data();
    void collect_gross_lev_data();
    
    // Data transformation helpers
    PyFolioItems::ReturnsData transform_returns_data() const;
    PyFolioItems::PositionsData transform_positions_data() const;
    PyFolioItems::TransactionsData transform_transactions_data() const;
    PyFolioItems::GrossLevData transform_gross_lev_data() const;
    
    // Utility methods
    std::string format_datetime(const std::chrono::system_clock::time_point& dt) const;
    std::chrono::system_clock::time_point localize_to_utc(const std::chrono::system_clock::time_point& dt) const;
    
    // Statistics calculation helpers
    double calculate_total_return() const;
    double calculate_annualized_return() const;
    double calculate_volatility() const;
    double calculate_sharpe_ratio() const;
    double calculate_max_drawdown() const;
    double calculate_calmar_ratio() const;
    
    // Validation helpers
    bool validate_returns_data() const;
    bool validate_positions_data() const;
    bool validate_transactions_data() const;
    bool validate_gross_lev_data() const;
    
    // Export helpers
    void write_csv_header(std::ofstream& file, const std::vector<std::string>& headers) const;
    void write_csv_row(std::ofstream& file, const std::vector<std::string>& values) const;
    
    // Error handling
    void handle_child_analyzer_error(const std::string& analyzer_name, const std::exception& e) const;
    void log_warning(const std::string& message) const;
    void log_error(const std::string& message) const;
};

} // namespace analyzers
} // namespace backtrader

// Register the analyzer
REGISTER_ANALYZER(backtrader::analyzers::PyFolio);