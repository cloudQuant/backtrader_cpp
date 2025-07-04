#pragma once

#include "../cerebro.h"
#include "../strategy.h"
#include "../indicator.h"
#include "../observer.h"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace backtrader {
namespace plot {

/**
 * PlotScheme - Color and style scheme for plotting
 */
struct PlotScheme {
    // Colors
    std::string background = "#ffffff";
    std::string grid = "#cccccc";
    std::string text = "#000000";
    std::string up_candle = "#00ff00";
    std::string down_candle = "#ff0000";
    std::string volume = "#0000ff";
    std::string buy_signal = "#00ff00";
    std::string sell_signal = "#ff0000";
    
    // Line styles
    std::vector<std::string> line_colors = {"#0000ff", "#ff0000", "#00ff00", "#ff9900", "#9900ff"};
    std::vector<std::string> line_styles = {"solid", "dashed", "dotted", "dashdot"};
    std::vector<int> line_widths = {1, 2, 3};
    
    // Font settings
    std::string font_family = "Arial";
    int font_size = 10;
};

/**
 * PlotInfo - Plot configuration for indicators and data
 */
struct PlotInfo {
    bool plot = true;              // Whether to plot this item
    bool subplot = false;          // Plot in separate subplot
    std::string plotname;          // Custom plot name
    std::vector<std::string> plotlinelabels;  // Line labels
    double plotyhlines = 0.0;      // Horizontal reference lines
    bool plotyticks = true;        // Show Y-axis ticks
    std::string plotmaster;        // Master plot for synchronization
    
    // Style settings
    std::string color;
    std::string linestyle = "solid";
    int linewidth = 1;
    std::string marker;
    double markersize = 4.0;
    std::string fillcolor;
    double alpha = 1.0;
};

/**
 * Plot - Main plotting interface for backtrader
 * 
 * Provides plotting capabilities for strategies, indicators, and analysis results.
 * Supports multiple backends (matplotlib, plotly, custom).
 */
class Plot {
public:
    // Plot backends
    enum class Backend {
        MATPLOTLIB,
        PLOTLY,
        CUSTOM
    };
    
    // Plot configuration
    struct Config {
        Backend backend = Backend::MATPLOTLIB;
        PlotScheme scheme;
        std::string output_file;
        std::string output_format = "png";  // png, svg, pdf, html
        int width = 1200;
        int height = 800;
        int dpi = 100;
        bool show_plot = true;
        bool save_plot = false;
        std::string title;
        bool show_legend = true;
        bool show_grid = true;
        int max_subplots = 10;
    };

    Plot(const Config& config = Config{});
    virtual ~Plot() = default;

    // Main plotting methods
    void plot_cerebro(std::shared_ptr<Cerebro> cerebro);
    void plot_strategy(std::shared_ptr<Strategy> strategy);
    void plot_data(std::shared_ptr<AbstractDataBase> data, const PlotInfo& info = PlotInfo{});
    void plot_indicator(std::shared_ptr<Indicator> indicator, const PlotInfo& info = PlotInfo{});
    void plot_observer(std::shared_ptr<Observer> observer, const PlotInfo& info = PlotInfo{});
    
    // Subplot management
    void add_subplot(const std::string& name);
    void set_current_subplot(const std::string& name);
    void clear_subplot(const std::string& name);
    
    // Plot customization
    void set_title(const std::string& title);
    void set_xlabel(const std::string& label);
    void set_ylabel(const std::string& label, const std::string& subplot = "");
    void add_legend(const std::string& subplot = "");
    void add_grid(bool enable = true, const std::string& subplot = "");
    
    // Output methods
    void show();
    void save(const std::string& filename = "");
    void close();
    void clear();

    // Configuration
    void set_scheme(const PlotScheme& scheme);
    void set_backend(Backend backend);
    Config& get_config() { return config_; }

private:
    // Configuration
    Config config_;
    
    // Plot data storage
    struct PlotData {
        std::vector<double> x_data;
        std::vector<double> y_data;
        std::string label;
        PlotInfo info;
        std::string subplot_name;
    };
    
    std::vector<PlotData> plot_data_;
    std::map<std::string, int> subplot_indices_;
    
    // Backend-specific implementations
    class PlotBackend {
    public:
        virtual ~PlotBackend() = default;
        virtual void initialize(const Config& config) = 0;
        virtual void plot_line(const std::vector<double>& x, const std::vector<double>& y,
                              const PlotInfo& info, const std::string& subplot) = 0;
        virtual void plot_candlestick(const std::vector<double>& x,
                                     const std::vector<double>& open,
                                     const std::vector<double>& high,
                                     const std::vector<double>& low,
                                     const std::vector<double>& close,
                                     const PlotInfo& info) = 0;
        virtual void plot_volume(const std::vector<double>& x,
                                const std::vector<double>& volume,
                                const PlotInfo& info) = 0;
        virtual void add_subplot(const std::string& name) = 0;
        virtual void set_title(const std::string& title) = 0;
        virtual void set_labels(const std::string& xlabel, const std::string& ylabel) = 0;
        virtual void show() = 0;
        virtual void save(const std::string& filename) = 0;
        virtual void close() = 0;
        virtual void clear() = 0;
    };
    
    std::unique_ptr<PlotBackend> backend_;
    
    // Internal methods
    void initialize_backend();
    void extract_data_series(std::shared_ptr<AbstractDataBase> data);
    void extract_indicator_lines(std::shared_ptr<Indicator> indicator);
    void extract_observer_lines(std::shared_ptr<Observer> observer);
    
    // Data processing
    std::vector<double> extract_time_series(std::shared_ptr<AbstractDataBase> data);
    std::vector<double> extract_price_series(std::shared_ptr<AbstractDataBase> data, int line_index);
    void prepare_candlestick_data(std::shared_ptr<AbstractDataBase> data);
    void prepare_volume_data(std::shared_ptr<AbstractDataBase> data);
    
    // Plot layout
    void organize_subplots();
    void apply_plot_scheme();
    void setup_plot_info(PlotInfo& info, const std::string& default_name);
    
    // Utility methods
    std::string generate_default_label(const std::string& base_name, int index);
    PlotInfo merge_plot_info(const PlotInfo& default_info, const PlotInfo& custom_info);
};

/**
 * Finance - Financial plotting utilities
 * 
 * Specialized plotting functions for financial data visualization.
 */
class Finance {
public:
    // Candlestick plotting
    static void plot_candlestick(Plot& plot,
                                 std::shared_ptr<AbstractDataBase> data,
                                 const PlotInfo& info = PlotInfo{});
    
    // Volume plotting
    static void plot_volume(Plot& plot,
                           std::shared_ptr<AbstractDataBase> data,
                           const PlotInfo& info = PlotInfo{});
    
    // Technical indicators
    static void plot_moving_average(Plot& plot,
                                   std::shared_ptr<Indicator> ma,
                                   const PlotInfo& info = PlotInfo{});
    
    static void plot_bollinger_bands(Plot& plot,
                                    std::shared_ptr<Indicator> bb,
                                    const PlotInfo& info = PlotInfo{});
    
    static void plot_rsi(Plot& plot,
                         std::shared_ptr<Indicator> rsi,
                         const PlotInfo& info = PlotInfo{});
    
    // Trade signals
    static void plot_buy_sell_signals(Plot& plot,
                                     std::shared_ptr<Observer> buysell,
                                     const PlotInfo& info = PlotInfo{});
    
    // Performance metrics
    static void plot_drawdown(Plot& plot,
                             std::shared_ptr<Observer> drawdown,
                             const PlotInfo& info = PlotInfo{});
    
    static void plot_returns(Plot& plot,
                            std::shared_ptr<Observer> returns,
                            const PlotInfo& info = PlotInfo{});
};

} // namespace plot
} // namespace backtrader