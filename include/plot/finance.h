#pragma once

#include <vector>
#include <string>
#include <memory>
#include <tuple>
#include <map>

namespace backtrader {
namespace plot {

/**
 * Color management for financial charts
 * 
 * Provides RGB color handling and manipulation for up/down market movements,
 * edge colors, and various styling options.
 */
struct Color {
    double r, g, b, a;  // RGBA components (0.0 to 1.0)
    
    Color(double red = 0.0, double green = 0.0, double blue = 0.0, double alpha = 1.0)
        : r(red), g(green), b(blue), a(alpha) {}
    
    Color(const std::string& hex_color, double alpha = 1.0);
    
    // Color manipulation
    Color shade(double factor) const;
    std::string to_hex() const;
    std::tuple<double, double, double, double> to_rgba() const;
    
    // Predefined colors
    static Color black() { return Color(0.0, 0.0, 0.0, 1.0); }
    static Color red() { return Color(1.0, 0.0, 0.0, 1.0); }
    static Color green() { return Color(0.0, 1.0, 0.0, 1.0); }
    static Color blue() { return Color(0.0, 0.0, 1.0, 1.0); }
    static Color white() { return Color(1.0, 1.0, 1.0, 1.0); }
    static Color transparent() { return Color(0.0, 0.0, 0.0, 0.0); }
};

/**
 * Base chart data structure for OHLC data
 */
struct ChartData {
    std::vector<double> x;       // X-axis coordinates
    std::vector<double> opens;   // Opening prices
    std::vector<double> highs;   // High prices
    std::vector<double> lows;    // Low prices
    std::vector<double> closes;  // Closing prices
    std::vector<double> volumes; // Volume data (optional)
    
    // Validation
    bool is_valid() const;
    size_t size() const { return x.size(); }
    void clear();
    
    // Data manipulation
    void normalize_x();
    std::pair<double, double> get_price_range() const;
    std::pair<double, double> get_volume_range() const;
};

/**
 * Rendering context for chart elements
 * 
 * Contains information about the current rendering state,
 * including coordinate transformations and styling options.
 */
struct RenderContext {
    double x_min, x_max;     // X-axis range
    double y_min, y_max;     // Y-axis range
    double width, height;    // Canvas dimensions
    double scaling = 1.0;    // Scaling factor
    double bottom = 0.0;     // Bottom offset
    
    // Coordinate transformation
    std::pair<double, double> transform_point(double x, double y) const;
    std::vector<std::pair<double, double>> transform_points(
        const std::vector<std::pair<double, double>>& points) const;
};

/**
 * Chart element collection
 * 
 * Represents a collection of drawable chart elements (bars, lines, etc.)
 * with associated styling and positioning information.
 */
class ChartElementCollection {
public:
    virtual ~ChartElementCollection() = default;
    
    // Rendering interface
    virtual void render(const RenderContext& context) const = 0;
    virtual void update_bounds(double& x_min, double& x_max, double& y_min, double& y_max) const = 0;
    
    // Legend support
    virtual void render_legend(const RenderContext& legend_context) const = 0;
    
    // Styling
    void set_z_order(double z_order) { z_order_ = z_order; }
    double get_z_order() const { return z_order_; }
    
    void set_label(const std::string& label) { label_ = label; }
    const std::string& get_label() const { return label_; }
    
protected:
    double z_order_ = 1.0;
    std::string label_ = "_nolegend";
};

/**
 * CandlestickPlotHandler - Handles OHLC candlestick chart rendering
 * 
 * This class manages the rendering of candlestick charts with customizable
 * colors for up/down movements, edge colors, and tick marks.
 */
class CandlestickPlotHandler {
public:
    // Configuration parameters
    struct Params {
        Color color_up = Color::black();      // Up candle color
        Color color_down = Color::red();      // Down candle color
        Color edge_up;                        // Up edge color (auto if not set)
        Color edge_down;                      // Down edge color (auto if not set)
        Color tick_up;                        // Up tick color (auto if not set)
        Color tick_down;                      // Down tick color (auto if not set)
        
        double width = 1.0;                   // Candle width
        double tick_width = 1.0;              // Tick line width
        double edge_adjust = 0.05;            // Edge adjustment factor
        double edge_shading = -10.0;          // Edge shading percentage
        double alpha = 1.0;                   // Transparency
        
        bool fill_up = true;                  // Fill up candles
        bool fill_down = true;                // Fill down candles
        
        std::string label = "_nolegend";      // Legend label
    };
    
    CandlestickPlotHandler(const Params& params = Params{});
    virtual ~CandlestickPlotHandler() = default;
    
    // Main rendering method
    std::pair<std::shared_ptr<ChartElementCollection>, std::shared_ptr<ChartElementCollection>>
    create_chart_elements(const ChartData& data) const;
    
    // Legend rendering
    std::pair<std::shared_ptr<ChartElementCollection>, std::shared_ptr<ChartElementCollection>>
    create_legend_elements(const RenderContext& context) const;
    
    // Direct plotting function (convenience)
    static std::pair<std::shared_ptr<ChartElementCollection>, std::shared_ptr<ChartElementCollection>>
    plot_candlestick(const ChartData& data, const Params& params = Params{});
    
protected:
    Params params_;
    
    // Legend data
    static const std::vector<double> legend_opens_;
    static const std::vector<double> legend_highs_;
    static const std::vector<double> legend_lows_;
    static const std::vector<double> legend_closes_;
    
private:
    // Color management
    void setup_colors();
    Color get_up_color(bool fill) const;
    Color get_down_color(bool fill) const;
    Color get_up_edge_color() const;
    Color get_down_edge_color() const;
    Color get_up_tick_color() const;
    Color get_down_tick_color() const;
    
    // Element creation helpers
    std::vector<std::vector<std::pair<double, double>>> create_candle_polygons(
        const ChartData& data, const RenderContext& context) const;
    std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> create_tick_lines(
        const ChartData& data, const RenderContext& context) const;
    std::vector<Color> get_candle_colors(const ChartData& data, bool for_fill) const;
    std::vector<Color> get_tick_colors(const ChartData& data) const;
};

/**
 * VolumePlotHandler - Handles volume bar chart rendering
 */
class VolumePlotHandler {
public:
    struct Params {
        Color color_up = Color::black();      // Up volume color
        Color color_down = Color::red();      // Down volume color
        Color edge_up;                        // Up edge color (auto if not set)
        Color edge_down;                      // Down edge color (auto if not set)
        
        double edge_shading = -5.0;           // Edge shading percentage
        double edge_adjust = 0.05;            // Edge adjustment factor
        double width = 1.0;                   // Bar width
        double alpha = 1.0;                   // Transparency
        
        std::string label = "_nolegend";      // Legend label
    };
    
    VolumePlotHandler(const Params& params = Params{});
    virtual ~VolumePlotHandler() = default;
    
    // Main rendering method
    std::shared_ptr<ChartElementCollection> create_chart_elements(const ChartData& data) const;
    
    // Legend rendering
    std::shared_ptr<ChartElementCollection> create_legend_elements(const RenderContext& context) const;
    
    // Direct plotting function
    static std::shared_ptr<ChartElementCollection> plot_volume(
        const ChartData& data, const Params& params = Params{});
    
protected:
    Params params_;
    
    // Legend data
    static const std::vector<double> legend_volumes_;
    static const std::vector<double> legend_opens_;
    static const std::vector<double> legend_closes_;
    
private:
    void setup_colors();
    std::vector<std::vector<std::pair<double, double>>> create_volume_bars(
        const ChartData& data, const RenderContext& context) const;
    std::vector<Color> get_volume_colors(const ChartData& data) const;
};

/**
 * OHLCPlotHandler - Traditional OHLC bar chart rendering
 */
class OHLCPlotHandler {
public:
    struct Params {
        Color color_up = Color::black();      // Up bar color
        Color color_down = Color::red();      // Down bar color
        
        double width = 1.0;                   // Bar line width
        double tick_width = 0.5;              // Tick width
        double alpha = 1.0;                   // Transparency
        
        std::string label = "_nolegend";      // Legend label
    };
    
    OHLCPlotHandler(const Params& params = Params{});
    virtual ~OHLCPlotHandler() = default;
    
    // Main rendering method
    std::tuple<std::shared_ptr<ChartElementCollection>, 
               std::shared_ptr<ChartElementCollection>,
               std::shared_ptr<ChartElementCollection>>
    create_chart_elements(const ChartData& data) const;
    
    // Legend rendering
    std::tuple<std::shared_ptr<ChartElementCollection>,
               std::shared_ptr<ChartElementCollection>,
               std::shared_ptr<ChartElementCollection>>
    create_legend_elements(const RenderContext& context) const;
    
    // Direct plotting function
    static std::tuple<std::shared_ptr<ChartElementCollection>,
                      std::shared_ptr<ChartElementCollection>,
                      std::shared_ptr<ChartElementCollection>>
    plot_ohlc(const ChartData& data, const Params& params = Params{});
    
protected:
    Params params_;
    
    // Legend data
    static const std::vector<double> legend_opens_;
    static const std::vector<double> legend_highs_;
    static const std::vector<double> legend_lows_;
    static const std::vector<double> legend_closes_;
    
private:
    void setup_colors();
    std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> create_bar_lines(
        const ChartData& data, const RenderContext& context) const;
    std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> create_open_ticks(
        const ChartData& data, const RenderContext& context) const;
    std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> create_close_ticks(
        const ChartData& data, const RenderContext& context) const;
    std::vector<Color> get_bar_colors(const ChartData& data) const;
};

/**
 * LineOnClosePlotHandler - Simple line plot on closing prices
 */
class LineOnClosePlotHandler {
public:
    struct Params {
        Color color = Color::black();         // Line color
        double width = 1.0;                   // Line width
        double alpha = 1.0;                   // Transparency
        std::string label = "_nolegend";      // Legend label
    };
    
    LineOnClosePlotHandler(const Params& params = Params{});
    virtual ~LineOnClosePlotHandler() = default;
    
    // Main rendering method
    std::shared_ptr<ChartElementCollection> create_chart_elements(const ChartData& data) const;
    
    // Legend rendering
    std::shared_ptr<ChartElementCollection> create_legend_elements(const RenderContext& context) const;
    
    // Direct plotting function
    static std::shared_ptr<ChartElementCollection> plot_line_on_close(
        const ChartData& data, const Params& params = Params{});
    
protected:
    Params params_;
    
    // Legend data
    static const std::vector<double> legend_closes_;
    
private:
    std::vector<std::pair<double, double>> create_line_points(
        const ChartData& data, const RenderContext& context) const;
};

/**
 * Concrete chart element implementations
 */

/**
 * Polygon collection for candlestick bodies
 */
class PolygonCollection : public ChartElementCollection {
public:
    PolygonCollection(const std::vector<std::vector<std::pair<double, double>>>& polygons,
                      const std::vector<Color>& face_colors,
                      const std::vector<Color>& edge_colors,
                      double line_width = 0.5);
    
    void render(const RenderContext& context) const override;
    void update_bounds(double& x_min, double& x_max, double& y_min, double& y_max) const override;
    void render_legend(const RenderContext& legend_context) const override;
    
private:
    std::vector<std::vector<std::pair<double, double>>> polygons_;
    std::vector<Color> face_colors_;
    std::vector<Color> edge_colors_;
    double line_width_;
};

/**
 * Line collection for ticks and OHLC bars
 */
class LineCollection : public ChartElementCollection {
public:
    LineCollection(const std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>>& lines,
                   const std::vector<Color>& colors,
                   double line_width = 1.0);
    
    void render(const RenderContext& context) const override;
    void update_bounds(double& x_min, double& x_max, double& y_min, double& y_max) const override;
    void render_legend(const RenderContext& legend_context) const override;
    
private:
    std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> lines_;
    std::vector<Color> colors_;
    double line_width_;
};

/**
 * Single line for close price plots
 */
class Line : public ChartElementCollection {
public:
    Line(const std::vector<std::pair<double, double>>& points,
         const Color& color,
         double line_width = 1.0);
    
    void render(const RenderContext& context) const override;
    void update_bounds(double& x_min, double& x_max, double& y_min, double& y_max) const override;
    void render_legend(const RenderContext& legend_context) const override;
    
private:
    std::vector<std::pair<double, double>> points_;
    Color color_;
    double line_width_;
};

/**
 * Utility functions
 */
namespace finance_utils {
    // Color parsing and manipulation
    Color parse_color(const std::string& color_spec);
    Color shade_color(const Color& color, double shade_factor);
    
    // Data validation
    bool validate_ohlc_data(const ChartData& data);
    bool validate_volume_data(const ChartData& data);
    
    // Chart data utilities
    ChartData create_sample_data(size_t num_points, double start_price = 100.0);
    std::pair<double, double> get_data_bounds_x(const ChartData& data);
    std::pair<double, double> get_data_bounds_y(const ChartData& data);
    
    // Coordinate helpers
    std::vector<double> generate_x_coordinates(size_t num_points, double start = 0.0, double step = 1.0);
    
    // Legend utilities
    RenderContext create_legend_context(double x, double y, double width, double height);
}

} // namespace plot
} // namespace backtrader