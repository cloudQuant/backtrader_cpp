#include "../../include/plot/finance.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <stdexcept>

namespace backtrader {
namespace plot {

// Static legend data definitions
const std::vector<double> CandlestickPlotHandler::legend_opens_ = {0.50, 0.50, 0.50};
const std::vector<double> CandlestickPlotHandler::legend_highs_ = {1.00, 1.00, 1.00};
const std::vector<double> CandlestickPlotHandler::legend_lows_ = {0.00, 0.00, 0.00};
const std::vector<double> CandlestickPlotHandler::legend_closes_ = {0.80, 0.00, 1.00};

const std::vector<double> VolumePlotHandler::legend_volumes_ = {0.5, 1.0, 0.75};
const std::vector<double> VolumePlotHandler::legend_opens_ = {0, 1, 0};
const std::vector<double> VolumePlotHandler::legend_closes_ = {1, 0, 1};

const std::vector<double> OHLCPlotHandler::legend_opens_ = {0.50, 0.50, 0.50};
const std::vector<double> OHLCPlotHandler::legend_highs_ = {1.00, 1.00, 1.00};
const std::vector<double> OHLCPlotHandler::legend_lows_ = {0.00, 0.00, 0.00};
const std::vector<double> OHLCPlotHandler::legend_closes_ = {0.80, 0.20, 0.90};

const std::vector<double> LineOnClosePlotHandler::legend_closes_ = {0.00, 0.66, 0.33, 1.00};

// Color implementation
Color::Color(const std::string& hex_color, double alpha) : a(alpha) {
    // Parse hex color (e.g., "#FF0000" or "FF0000")
    std::string hex = hex_color;
    if (hex[0] == '#') {
        hex = hex.substr(1);
    }
    
    if (hex.length() != 6) {
        throw std::invalid_argument("Invalid hex color format");
    }
    
    unsigned int rgb;
    std::stringstream ss;
    ss << std::hex << hex;
    ss >> rgb;
    
    r = ((rgb >> 16) & 0xFF) / 255.0;
    g = ((rgb >> 8) & 0xFF) / 255.0;
    b = (rgb & 0xFF) / 255.0;
}

Color Color::shade(double factor) const {
    if (factor > 0) {
        // Lighten
        return Color(
            r + (1.0 - r) * factor / 100.0,
            g + (1.0 - g) * factor / 100.0,
            b + (1.0 - b) * factor / 100.0,
            a
        );
    } else {
        // Darken
        factor = -factor;
        return Color(
            r * (1.0 - factor / 100.0),
            g * (1.0 - factor / 100.0),
            b * (1.0 - factor / 100.0),
            a
        );
    }
}

std::string Color::to_hex() const {
    std::stringstream ss;
    ss << "#" << std::hex << std::setfill('0')
       << std::setw(2) << static_cast<int>(r * 255)
       << std::setw(2) << static_cast<int>(g * 255)
       << std::setw(2) << static_cast<int>(b * 255);
    return ss.str();
}

std::tuple<double, double, double, double> Color::to_rgba() const {
    return std::make_tuple(r, g, b, a);
}

// ChartData implementation
bool ChartData::is_valid() const {
    size_t n = x.size();
    return n > 0 && 
           opens.size() == n && 
           highs.size() == n && 
           lows.size() == n && 
           closes.size() == n &&
           (volumes.empty() || volumes.size() == n);
}

void ChartData::clear() {
    x.clear();
    opens.clear();
    highs.clear();
    lows.clear();
    closes.clear();
    volumes.clear();
}

void ChartData::normalize_x() {
    for (size_t i = 0; i < x.size(); ++i) {
        x[i] = static_cast<double>(i);
    }
}

std::pair<double, double> ChartData::get_price_range() const {
    if (highs.empty() || lows.empty()) {
        return {0.0, 0.0};
    }
    
    auto min_it = std::min_element(lows.begin(), lows.end());
    auto max_it = std::max_element(highs.begin(), highs.end());
    
    return {*min_it, *max_it};
}

std::pair<double, double> ChartData::get_volume_range() const {
    if (volumes.empty()) {
        return {0.0, 0.0};
    }
    
    auto min_it = std::min_element(volumes.begin(), volumes.end());
    auto max_it = std::max_element(volumes.begin(), volumes.end());
    
    return {*min_it, *max_it};
}

// RenderContext implementation
std::pair<double, double> RenderContext::transform_point(double x, double y) const {
    double transformed_x = (x - x_min) / (x_max - x_min) * width;
    double transformed_y = (y - y_min) / (y_max - y_min) * height * scaling + bottom;
    return {transformed_x, transformed_y};
}

std::vector<std::pair<double, double>> RenderContext::transform_points(
    const std::vector<std::pair<double, double>>& points) const {
    
    std::vector<std::pair<double, double>> transformed;
    transformed.reserve(points.size());
    
    for (const auto& point : points) {
        transformed.push_back(transform_point(point.first, point.second));
    }
    
    return transformed;
}

// CandlestickPlotHandler implementation
CandlestickPlotHandler::CandlestickPlotHandler(const Params& params) : params_(params) {
    setup_colors();
}

std::pair<std::shared_ptr<ChartElementCollection>, std::shared_ptr<ChartElementCollection>>
CandlestickPlotHandler::create_chart_elements(const ChartData& data) const {
    
    if (!data.is_valid()) {
        throw std::invalid_argument("Invalid chart data for candlestick plot");
    }
    
    RenderContext context;
    context.x_min = 0;
    context.x_max = static_cast<double>(data.size());
    auto price_range = data.get_price_range();
    context.y_min = price_range.first;
    context.y_max = price_range.second;
    
    // Create candle body polygons
    auto polygons = create_candle_polygons(data, context);
    auto face_colors = get_candle_colors(data, true);
    auto edge_colors = get_candle_colors(data, false);
    
    auto bar_collection = std::make_shared<PolygonCollection>(
        polygons, face_colors, edge_colors, 0.5);
    bar_collection->set_label(params_.label);
    
    // Create tick lines
    auto tick_lines = create_tick_lines(data, context);
    auto tick_colors = get_tick_colors(data);
    
    auto tick_collection = std::make_shared<LineCollection>(
        tick_lines, tick_colors, params_.tick_width);
    tick_collection->set_label("_nolegend");
    tick_collection->set_z_order(bar_collection->get_z_order() * 0.9999);
    
    return {bar_collection, tick_collection};
}

std::pair<std::shared_ptr<ChartElementCollection>, std::shared_ptr<ChartElementCollection>>
CandlestickPlotHandler::create_legend_elements(const RenderContext& context) const {
    
    ChartData legend_data;
    legend_data.x.resize(legend_opens_.size());
    std::iota(legend_data.x.begin(), legend_data.x.end(), 0.0);
    legend_data.opens = legend_opens_;
    legend_data.highs = legend_highs_;
    legend_data.lows = legend_lows_;
    legend_data.closes = legend_closes_;
    
    return create_chart_elements(legend_data);
}

std::pair<std::shared_ptr<ChartElementCollection>, std::shared_ptr<ChartElementCollection>>
CandlestickPlotHandler::plot_candlestick(const ChartData& data, const Params& params) {
    CandlestickPlotHandler handler(params);
    return handler.create_chart_elements(data);
}

void CandlestickPlotHandler::setup_colors() {
    // Setup edge colors if not specified
    if (params_.edge_up.a == 0.0) {  // Assuming transparent means not set
        params_.edge_up = params_.color_up.shade(params_.edge_shading);
    }
    
    if (params_.edge_down.a == 0.0) {
        params_.edge_down = params_.color_down.shade(params_.edge_shading);
    }
    
    // Setup tick colors if not specified
    if (params_.tick_up.a == 0.0) {
        params_.tick_up = params_.edge_up;
    }
    
    if (params_.tick_down.a == 0.0) {
        params_.tick_down = params_.edge_down;
    }
}

Color CandlestickPlotHandler::get_up_color(bool fill) const {
    if (fill && params_.fill_up) {
        return params_.color_up;
    } else if (!fill) {
        return params_.edge_up;
    } else {
        return Color::transparent();
    }
}

Color CandlestickPlotHandler::get_down_color(bool fill) const {
    if (fill && params_.fill_down) {
        return params_.color_down;
    } else if (!fill) {
        return params_.edge_down;
    } else {
        return Color::transparent();
    }
}

Color CandlestickPlotHandler::get_up_edge_color() const {
    return params_.edge_up;
}

Color CandlestickPlotHandler::get_down_edge_color() const {
    return params_.edge_down;
}

Color CandlestickPlotHandler::get_up_tick_color() const {
    return params_.tick_up;
}

Color CandlestickPlotHandler::get_down_tick_color() const {
    return params_.tick_down;
}

std::vector<std::vector<std::pair<double, double>>> CandlestickPlotHandler::create_candle_polygons(
    const ChartData& data, const RenderContext& context) const {
    
    std::vector<std::vector<std::pair<double, double>>> polygons;
    polygons.reserve(data.size());
    
    double delta = params_.width / 2.0 - params_.edge_adjust;
    
    for (size_t i = 0; i < data.size(); ++i) {
        double x = data.x[i];
        double open = data.opens[i];
        double close = data.closes[i];
        
        double left = x - delta;
        double right = x + delta;
        
        // Create rectangular polygon for candle body
        std::vector<std::pair<double, double>> polygon = {
            {left, open},
            {left, close},
            {right, close},
            {right, open}
        };
        
        polygons.push_back(polygon);
    }
    
    return polygons;
}

std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> 
CandlestickPlotHandler::create_tick_lines(const ChartData& data, const RenderContext& context) const {
    
    std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> lines;
    lines.reserve(data.size() * 2);  // Up and down ticks for each candle
    
    for (size_t i = 0; i < data.size(); ++i) {
        double x = data.x[i];
        double open = data.opens[i];
        double high = data.highs[i];
        double low = data.lows[i];
        double close = data.closes[i];
        
        // Upper tick (from max(open, close) to high)
        double body_top = std::max(open, close);
        lines.push_back({{x, body_top}, {x, high}});
        
        // Lower tick (from low to min(open, close))
        double body_bottom = std::min(open, close);
        lines.push_back({{x, low}, {x, body_bottom}});
    }
    
    return lines;
}

std::vector<Color> CandlestickPlotHandler::get_candle_colors(const ChartData& data, bool for_fill) const {
    std::vector<Color> colors;
    colors.reserve(data.size());
    
    for (size_t i = 0; i < data.size(); ++i) {
        bool is_up = data.opens[i] < data.closes[i];
        
        if (for_fill) {
            colors.push_back(is_up ? get_up_color(true) : get_down_color(true));
        } else {
            colors.push_back(is_up ? get_up_edge_color() : get_down_edge_color());
        }
    }
    
    return colors;
}

std::vector<Color> CandlestickPlotHandler::get_tick_colors(const ChartData& data) const {
    std::vector<Color> colors;
    colors.reserve(data.size() * 2);  // Two ticks per candle
    
    for (size_t i = 0; i < data.size(); ++i) {
        bool is_up = data.opens[i] < data.closes[i];
        Color tick_color = is_up ? get_up_tick_color() : get_down_tick_color();
        
        // Add color for both upper and lower ticks
        colors.push_back(tick_color);
        colors.push_back(tick_color);
    }
    
    return colors;
}

// VolumePlotHandler implementation
VolumePlotHandler::VolumePlotHandler(const Params& params) : params_(params) {
    setup_colors();
}

std::shared_ptr<ChartElementCollection> VolumePlotHandler::create_chart_elements(const ChartData& data) const {
    if (!data.is_valid() || data.volumes.empty()) {
        throw std::invalid_argument("Invalid chart data for volume plot");
    }
    
    RenderContext context;
    context.x_min = 0;
    context.x_max = static_cast<double>(data.size());
    auto volume_range = data.get_volume_range();
    context.y_min = volume_range.first;
    context.y_max = volume_range.second;
    
    auto polygons = create_volume_bars(data, context);
    auto colors = get_volume_colors(data);
    
    auto bar_collection = std::make_shared<PolygonCollection>(
        polygons, colors, colors, 0.5);
    bar_collection->set_label(params_.label);
    
    return bar_collection;
}

std::shared_ptr<ChartElementCollection> VolumePlotHandler::create_legend_elements(const RenderContext& context) const {
    ChartData legend_data;
    legend_data.x.resize(legend_volumes_.size());
    std::iota(legend_data.x.begin(), legend_data.x.end(), 0.0);
    legend_data.opens = legend_opens_;
    legend_data.closes = legend_closes_;
    legend_data.volumes = legend_volumes_;
    
    return create_chart_elements(legend_data);
}

std::shared_ptr<ChartElementCollection> VolumePlotHandler::plot_volume(
    const ChartData& data, const Params& params) {
    VolumePlotHandler handler(params);
    return handler.create_chart_elements(data);
}

void VolumePlotHandler::setup_colors() {
    if (params_.edge_up.a == 0.0) {
        params_.edge_up = params_.color_up.shade(params_.edge_shading);
    }
    
    if (params_.edge_down.a == 0.0) {
        params_.edge_down = params_.color_down.shade(params_.edge_shading);
    }
}

std::vector<std::vector<std::pair<double, double>>> VolumePlotHandler::create_volume_bars(
    const ChartData& data, const RenderContext& context) const {
    
    std::vector<std::vector<std::pair<double, double>>> polygons;
    polygons.reserve(data.size());
    
    double delta = params_.width / 2.0 - params_.edge_adjust;
    
    for (size_t i = 0; i < data.size(); ++i) {
        double x = data.x[i];
        double volume = data.volumes[i];
        
        double left = x - delta;
        double right = x + delta;
        
        std::vector<std::pair<double, double>> polygon = {
            {left, 0.0},
            {left, volume},
            {right, volume},
            {right, 0.0}
        };
        
        polygons.push_back(polygon);
    }
    
    return polygons;
}

std::vector<Color> VolumePlotHandler::get_volume_colors(const ChartData& data) const {
    std::vector<Color> colors;
    colors.reserve(data.size());
    
    for (size_t i = 0; i < data.size(); ++i) {
        bool is_up = data.opens[i] < data.closes[i];
        colors.push_back(is_up ? params_.color_up : params_.color_down);
    }
    
    return colors;
}

// PolygonCollection implementation
PolygonCollection::PolygonCollection(
    const std::vector<std::vector<std::pair<double, double>>>& polygons,
    const std::vector<Color>& face_colors,
    const std::vector<Color>& edge_colors,
    double line_width)
    : polygons_(polygons), face_colors_(face_colors), edge_colors_(edge_colors), line_width_(line_width) {}

void PolygonCollection::render(const RenderContext& context) const {
    // Placeholder implementation - in a real implementation, this would
    // interface with a graphics library like Cairo, Skia, or OpenGL
    std::cout << "Rendering " << polygons_.size() << " polygons" << std::endl;
    for (size_t i = 0; i < polygons_.size(); ++i) {
        std::cout << "  Polygon " << i << " with " << polygons_[i].size() << " vertices";
        if (i < face_colors_.size()) {
            std::cout << ", face color: " << face_colors_[i].to_hex();
        }
        if (i < edge_colors_.size()) {
            std::cout << ", edge color: " << edge_colors_[i].to_hex();
        }
        std::cout << std::endl;
    }
}

void PolygonCollection::update_bounds(double& x_min, double& x_max, double& y_min, double& y_max) const {
    for (const auto& polygon : polygons_) {
        for (const auto& point : polygon) {
            x_min = std::min(x_min, point.first);
            x_max = std::max(x_max, point.first);
            y_min = std::min(y_min, point.second);
            y_max = std::max(y_max, point.second);
        }
    }
}

void PolygonCollection::render_legend(const RenderContext& legend_context) const {
    std::cout << "Rendering polygon legend" << std::endl;
}

// LineCollection implementation
LineCollection::LineCollection(
    const std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>>& lines,
    const std::vector<Color>& colors,
    double line_width)
    : lines_(lines), colors_(colors), line_width_(line_width) {}

void LineCollection::render(const RenderContext& context) const {
    std::cout << "Rendering " << lines_.size() << " lines" << std::endl;
    for (size_t i = 0; i < lines_.size(); ++i) {
        const auto& line = lines_[i];
        std::cout << "  Line " << i << " from (" << line.first.first << "," << line.first.second 
                  << ") to (" << line.second.first << "," << line.second.second << ")";
        if (i < colors_.size()) {
            std::cout << ", color: " << colors_[i].to_hex();
        }
        std::cout << std::endl;
    }
}

void LineCollection::update_bounds(double& x_min, double& x_max, double& y_min, double& y_max) const {
    for (const auto& line : lines_) {
        x_min = std::min(x_min, std::min(line.first.first, line.second.first));
        x_max = std::max(x_max, std::max(line.first.first, line.second.first));
        y_min = std::min(y_min, std::min(line.first.second, line.second.second));
        y_max = std::max(y_max, std::max(line.first.second, line.second.second));
    }
}

void LineCollection::render_legend(const RenderContext& legend_context) const {
    std::cout << "Rendering line legend" << std::endl;
}

// Line implementation
Line::Line(const std::vector<std::pair<double, double>>& points,
           const Color& color,
           double line_width)
    : points_(points), color_(color), line_width_(line_width) {}

void Line::render(const RenderContext& context) const {
    std::cout << "Rendering line with " << points_.size() << " points, color: " << color_.to_hex() << std::endl;
}

void Line::update_bounds(double& x_min, double& x_max, double& y_min, double& y_max) const {
    for (const auto& point : points_) {
        x_min = std::min(x_min, point.first);
        x_max = std::max(x_max, point.first);
        y_min = std::min(y_min, point.second);
        y_max = std::max(y_max, point.second);
    }
}

void Line::render_legend(const RenderContext& legend_context) const {
    std::cout << "Rendering line legend" << std::endl;
}

// Utility functions
namespace finance_utils {

Color parse_color(const std::string& color_spec) {
    if (color_spec == "k" || color_spec == "black") return Color::black();
    if (color_spec == "r" || color_spec == "red") return Color::red();
    if (color_spec == "g" || color_spec == "green") return Color::green();
    if (color_spec == "b" || color_spec == "blue") return Color::blue();
    if (color_spec == "w" || color_spec == "white") return Color::white();
    
    if (color_spec[0] == '#') {
        return Color(color_spec);
    }
    
    throw std::invalid_argument("Unknown color specification: " + color_spec);
}

Color shade_color(const Color& color, double shade_factor) {
    return color.shade(shade_factor);
}

bool validate_ohlc_data(const ChartData& data) {
    if (!data.is_valid()) return false;
    
    for (size_t i = 0; i < data.size(); ++i) {
        if (data.highs[i] < std::max(data.opens[i], data.closes[i]) ||
            data.lows[i] > std::min(data.opens[i], data.closes[i])) {
            return false;
        }
    }
    
    return true;
}

bool validate_volume_data(const ChartData& data) {
    if (!data.is_valid() || data.volumes.empty()) return false;
    
    for (double volume : data.volumes) {
        if (volume < 0.0) return false;
    }
    
    return true;
}

ChartData create_sample_data(size_t num_points, double start_price) {
    ChartData data;
    data.x.resize(num_points);
    data.opens.resize(num_points);
    data.highs.resize(num_points);
    data.lows.resize(num_points);
    data.closes.resize(num_points);
    data.volumes.resize(num_points);
    
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    
    double price = start_price;
    for (size_t i = 0; i < num_points; ++i) {
        data.x[i] = static_cast<double>(i);
        
        double change = (std::rand() % 2001 - 1000) / 10000.0 * price; // ±10% max change
        data.opens[i] = price;
        price += change;
        data.closes[i] = price;
        
        data.highs[i] = std::max(data.opens[i], data.closes[i]) + 
                        (std::rand() % 500) / 10000.0 * price;
        data.lows[i] = std::min(data.opens[i], data.closes[i]) - 
                       (std::rand() % 500) / 10000.0 * price;
        
        data.volumes[i] = 1000 + std::rand() % 9000; // Random volume 1000-10000
    }
    
    return data;
}

std::pair<double, double> get_data_bounds_x(const ChartData& data) {
    if (data.x.empty()) return {0.0, 0.0};
    auto minmax = std::minmax_element(data.x.begin(), data.x.end());
    return {*minmax.first, *minmax.second};
}

std::pair<double, double> get_data_bounds_y(const ChartData& data) {
    return data.get_price_range();
}

std::vector<double> generate_x_coordinates(size_t num_points, double start, double step) {
    std::vector<double> coords;
    coords.reserve(num_points);
    
    for (size_t i = 0; i < num_points; ++i) {
        coords.push_back(start + i * step);
    }
    
    return coords;
}

RenderContext create_legend_context(double x, double y, double width, double height) {
    RenderContext context;
    context.x_min = x;
    context.x_max = x + width;
    context.y_min = y;
    context.y_max = y + height;
    context.width = width;
    context.height = height;
    context.scaling = 1.0;
    context.bottom = y;
    
    return context;
}

} // namespace finance_utils

} // namespace plot
} // namespace backtrader