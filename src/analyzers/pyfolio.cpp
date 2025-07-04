#include "../../include/analyzers/pyfolio.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <stdexcept>
#include <numeric>
#include <cmath>

namespace backtrader {
namespace analyzers {

PyFolio::PyFolio(const Params& params) : p(params) {
    // Initialize child analyzers
    initialize_child_analyzers();
    
    // Clear results
    combined_results.clear();
}

void PyFolio::start() {
    // Call parent start
    Analyzer::start();
    
    // Initialize child analyzers
    initialize_child_analyzers();
    
    // Setup analyzer relationships
    setup_analyzer_relationships();
    
    // Start all child analyzers
    if (_returns) _returns->start();
    if (_positions) _positions->start();
    if (_transactions) _transactions->start();
    if (_gross_lev) _gross_lev->start();
    
    // Clear results
    combined_results.clear();
}

void PyFolio::stop() {
    // Call parent stop
    Analyzer::stop();
    
    // Stop all child analyzers
    if (_returns) _returns->stop();
    if (_positions) _positions->stop();
    if (_transactions) _transactions->stop();
    if (_gross_lev) _gross_lev->stop();
    
    // Collect data from all child analyzers
    collect_returns_data();
    collect_positions_data();
    collect_transactions_data();
    collect_gross_lev_data();
}

AnalysisResult PyFolio::get_analysis() const {
    return combined_results;
}

PyFolio::PyFolioItems PyFolio::get_pf_items() const {
    PyFolioItems items;
    
    // Transform data from child analyzers
    items.returns = transform_returns_data();
    items.positions = transform_positions_data();
    items.transactions = transform_transactions_data();
    items.gross_lev = transform_gross_lev_data();
    
    return items;
}

void PyFolio::export_returns_csv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    
    // Write header
    file << "date,return\n";
    
    // Get returns data
    auto returns_data = transform_returns_data();
    
    // Write data
    for (size_t i = 0; i < returns_data.dates.size(); ++i) {
        file << format_datetime(returns_data.dates[i]) << "," 
             << std::fixed << std::setprecision(8) << returns_data.values[i] << "\n";
    }
    
    file.close();
}

void PyFolio::export_positions_csv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    
    // Get positions data
    auto positions_data = transform_positions_data();
    
    // Write header
    file << "date";
    for (const auto& header : positions_data.headers) {
        file << "," << header;
    }
    file << "\n";
    
    // Write data
    for (size_t i = 0; i < positions_data.dates.size(); ++i) {
        file << format_datetime(positions_data.dates[i]);
        if (i < positions_data.values.size()) {
            for (double value : positions_data.values[i]) {
                file << "," << std::fixed << std::setprecision(6) << value;
            }
        }
        file << "\n";
    }
    
    file.close();
}

void PyFolio::export_transactions_csv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    
    // Get transactions data
    auto transactions_data = transform_transactions_data();
    
    // Write header
    file << "date";
    for (const auto& header : transactions_data.headers) {
        file << "," << header;
    }
    file << "\n";
    
    // Write data
    for (size_t i = 0; i < transactions_data.dates.size(); ++i) {
        file << format_datetime(transactions_data.dates[i]);
        if (i < transactions_data.values.size()) {
            for (double value : transactions_data.values[i]) {
                file << "," << std::fixed << std::setprecision(6) << value;
            }
        }
        file << "\n";
    }
    
    file.close();
}

void PyFolio::export_gross_lev_csv(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    
    // Write header
    file << "date,gross_leverage\n";
    
    // Get gross leverage data
    auto gross_lev_data = transform_gross_lev_data();
    
    // Write data
    for (size_t i = 0; i < gross_lev_data.dates.size(); ++i) {
        file << format_datetime(gross_lev_data.dates[i]) << "," 
             << std::fixed << std::setprecision(6) << gross_lev_data.values[i] << "\n";
    }
    
    file.close();
}

void PyFolio::export_all_csv(const std::string& base_filename) const {
    // Export all data to separate CSV files
    export_returns_csv(base_filename + "_returns.csv");
    export_positions_csv(base_filename + "_positions.csv");
    export_transactions_csv(base_filename + "_transactions.csv");
    export_gross_lev_csv(base_filename + "_gross_lev.csv");
}

PyFolio::SummaryStatistics PyFolio::get_summary_statistics() const {
    SummaryStatistics stats;
    
    // Calculate summary statistics
    stats.total_return = calculate_total_return();
    stats.annualized_return = calculate_annualized_return();
    stats.volatility = calculate_volatility();
    stats.sharpe_ratio = calculate_sharpe_ratio();
    stats.max_drawdown = calculate_max_drawdown();
    stats.calmar_ratio = calculate_calmar_ratio();
    
    // Get date range
    auto returns_data = transform_returns_data();
    if (!returns_data.dates.empty()) {
        stats.start_date = returns_data.dates.front();
        stats.end_date = returns_data.dates.back();
        stats.duration = std::chrono::duration_cast<std::chrono::seconds>(
            stats.end_date - stats.start_date);
    } else {
        stats.start_date = std::chrono::system_clock::now();
        stats.end_date = std::chrono::system_clock::now();
        stats.duration = std::chrono::seconds(0);
    }
    
    // Transaction statistics
    auto transactions_data = transform_transactions_data();
    stats.num_trades = transactions_data.dates.size();
    
    // Calculate average trade size
    if (!transactions_data.values.empty()) {
        double total_trade_size = 0.0;
        size_t trade_count = 0;
        
        for (const auto& trade_row : transactions_data.values) {
            if (!trade_row.empty()) {
                total_trade_size += std::abs(trade_row[0]); // Assuming first column is trade size
                trade_count++;
            }
        }
        
        stats.avg_trade_size = trade_count > 0 ? total_trade_size / trade_count : 0.0;
    } else {
        stats.avg_trade_size = 0.0;
    }
    
    // Calculate hit ratio (placeholder - would need more detailed trade analysis)
    stats.hit_ratio = 0.0;
    
    return stats;
}

bool PyFolio::validate_data_consistency() const {
    return validate_returns_data() && 
           validate_positions_data() && 
           validate_transactions_data() && 
           validate_gross_lev_data();
}

std::vector<std::string> PyFolio::get_data_quality_issues() const {
    std::vector<std::string> issues;
    
    // Check each data source for issues
    if (!validate_returns_data()) {
        issues.push_back("Returns data validation failed");
    }
    
    if (!validate_positions_data()) {
        issues.push_back("Positions data validation failed");
    }
    
    if (!validate_transactions_data()) {
        issues.push_back("Transactions data validation failed");
    }
    
    if (!validate_gross_lev_data()) {
        issues.push_back("Gross leverage data validation failed");
    }
    
    return issues;
}

void PyFolio::initialize_child_analyzers() {
    try {
        // Create TimeReturn analyzer with timeframe parameters
        TimeReturn::Params returns_params;
        returns_params.timeframe = p.timeframe;
        returns_params.compression = p.compression;
        _returns = std::make_shared<TimeReturn>(returns_params);
        
        // Create PositionsValue analyzer with headers and cash
        PositionsValue::Params positions_params;
        positions_params.headers = true;
        positions_params.cash = true;
        _positions = std::make_shared<PositionsValue>(positions_params);
        
        // Create Transactions analyzer with headers
        Transactions::Params transactions_params;
        transactions_params.headers = true;
        _transactions = std::make_shared<Transactions>(transactions_params);
        
        // Create GrossLeverage analyzer
        GrossLeverage::Params gross_lev_params;
        _gross_lev = std::make_shared<GrossLeverage>(gross_lev_params);
        
    } catch (const std::exception& e) {
        handle_child_analyzer_error("initialization", e);
    }
}

void PyFolio::setup_analyzer_relationships() {
    // Set up parent-child relationships
    if (_returns) {
        _returns->strategy = strategy;
        _returns->datas = datas;
        _returns->data = data;
    }
    
    if (_positions) {
        _positions->strategy = strategy;
        _positions->datas = datas;
        _positions->data = data;
    }
    
    if (_transactions) {
        _transactions->strategy = strategy;
        _transactions->datas = datas;
        _transactions->data = data;
    }
    
    if (_gross_lev) {
        _gross_lev->strategy = strategy;
        _gross_lev->datas = datas;
        _gross_lev->data = data;
    }
}

void PyFolio::collect_returns_data() {
    try {
        if (_returns) {
            auto returns_analysis = _returns->get_analysis();
            combined_results["returns"] = returns_analysis;
        }
    } catch (const std::exception& e) {
        handle_child_analyzer_error("returns collection", e);
    }
}

void PyFolio::collect_positions_data() {
    try {
        if (_positions) {
            auto positions_analysis = _positions->get_analysis();
            combined_results["positions"] = positions_analysis;
        }
    } catch (const std::exception& e) {
        handle_child_analyzer_error("positions collection", e);
    }
}

void PyFolio::collect_transactions_data() {
    try {
        if (_transactions) {
            auto transactions_analysis = _transactions->get_analysis();
            combined_results["transactions"] = transactions_analysis;
        }
    } catch (const std::exception& e) {
        handle_child_analyzer_error("transactions collection", e);
    }
}

void PyFolio::collect_gross_lev_data() {
    try {
        if (_gross_lev) {
            auto gross_lev_analysis = _gross_lev->get_analysis();
            combined_results["gross_lev"] = gross_lev_analysis;
        }
    } catch (const std::exception& e) {
        handle_child_analyzer_error("gross leverage collection", e);
    }
}

PyFolio::PyFolioItems::ReturnsData PyFolio::transform_returns_data() const {
    PyFolioItems::ReturnsData returns_data;
    
    // TODO: Extract actual returns data from child analyzer
    // This is a placeholder implementation
    
    return returns_data;
}

PyFolio::PyFolioItems::PositionsData PyFolio::transform_positions_data() const {
    PyFolioItems::PositionsData positions_data;
    
    // TODO: Extract actual positions data from child analyzer
    // This is a placeholder implementation
    
    return positions_data;
}

PyFolio::PyFolioItems::TransactionsData PyFolio::transform_transactions_data() const {
    PyFolioItems::TransactionsData transactions_data;
    
    // TODO: Extract actual transactions data from child analyzer
    // This is a placeholder implementation
    
    return transactions_data;
}

PyFolio::PyFolioItems::GrossLevData PyFolio::transform_gross_lev_data() const {
    PyFolioItems::GrossLevData gross_lev_data;
    
    // TODO: Extract actual gross leverage data from child analyzer
    // This is a placeholder implementation
    
    return gross_lev_data;
}

std::string PyFolio::format_datetime(const std::chrono::system_clock::time_point& dt) const {
    // Convert time_point to time_t
    std::time_t time_t = std::chrono::system_clock::to_time_t(dt);
    
    // Convert to tm struct (UTC)
    std::tm* tm_ptr = std::gmtime(&time_t);
    
    // Format as ISO string with UTC timezone
    std::ostringstream oss;
    oss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S UTC");
    
    return oss.str();
}

std::chrono::system_clock::time_point PyFolio::localize_to_utc(const std::chrono::system_clock::time_point& dt) const {
    // For now, assume input is already in UTC
    // In a real implementation, this would handle timezone conversion
    return dt;
}

double PyFolio::calculate_total_return() const {
    // TODO: Calculate total return from returns data
    return 0.0;
}

double PyFolio::calculate_annualized_return() const {
    // TODO: Calculate annualized return
    return 0.0;
}

double PyFolio::calculate_volatility() const {
    // TODO: Calculate volatility from returns data
    return 0.0;
}

double PyFolio::calculate_sharpe_ratio() const {
    // TODO: Calculate Sharpe ratio
    return 0.0;
}

double PyFolio::calculate_max_drawdown() const {
    // TODO: Calculate maximum drawdown
    return 0.0;
}

double PyFolio::calculate_calmar_ratio() const {
    // TODO: Calculate Calmar ratio
    return 0.0;
}

bool PyFolio::validate_returns_data() const {
    // TODO: Validate returns data
    return true;
}

bool PyFolio::validate_positions_data() const {
    // TODO: Validate positions data
    return true;
}

bool PyFolio::validate_transactions_data() const {
    // TODO: Validate transactions data
    return true;
}

bool PyFolio::validate_gross_lev_data() const {
    // TODO: Validate gross leverage data
    return true;
}

void PyFolio::handle_child_analyzer_error(const std::string& analyzer_name, const std::exception& e) const {
    log_error("Error in " + analyzer_name + ": " + e.what());
}

void PyFolio::log_warning(const std::string& message) const {
    std::cout << "WARNING [PyFolio]: " << message << std::endl;
}

void PyFolio::log_error(const std::string& message) const {
    std::cerr << "ERROR [PyFolio]: " << message << std::endl;
}

} // namespace analyzers
} // namespace backtrader