#pragma once

#include <string>
#include <vector>
#include <map>

namespace backtrader {
namespace plot {

/**
 * PlotScheme - Comprehensive plotting color and style schemes
 * 
 * Provides predefined color schemes and styling options for financial plots.
 */
class PlotScheme {
public:
    // Predefined schemes
    enum class SchemeType {
        DEFAULT,
        DARK,
        LIGHT,
        PROFESSIONAL,
        COLORBLIND_FRIENDLY,
        CUSTOM
    };

    // Color definitions
    struct Colors {
        std::string background = "#ffffff";
        std::string grid = "#cccccc";
        std::string text = "#000000";
        std::string axes = "#000000";
        
        // Candlestick colors
        std::string up_candle = "#00aa00";
        std::string down_candle = "#aa0000";
        std::string up_wick = "#00aa00";
        std::string down_wick = "#aa0000";
        std::string up_edge = "#00aa00";
        std::string down_edge = "#aa0000";
        
        // Volume colors
        std::string volume_up = "#00aa0080";
        std::string volume_down = "#aa000080";
        
        // Signal colors
        std::string buy_signal = "#00ff00";
        std::string sell_signal = "#ff0000";
        std::string entry_signal = "#0000ff";
        std::string exit_signal = "#ff9900";
        
        // Indicator colors
        std::vector<std::string> line_colors = {
            "#0000ff", "#ff0000", "#00aa00", "#ff9900", 
            "#9900ff", "#00ffff", "#ffff00", "#ff00ff"
        };
        
        // Special colors
        std::string positive = "#00aa00";
        std::string negative = "#aa0000";
        std::string neutral = "#808080";
        std::string highlight = "#ffff00";
        std::string warning = "#ff9900";
        std::string error = "#ff0000";
    };

    // Style definitions
    struct Styles {
        std::vector<std::string> line_styles = {"solid", "dashed", "dotted", "dashdot"};
        std::vector<int> line_widths = {1, 2, 3, 4};
        std::vector<std::string> markers = {"o", "s", "^", "v", "<", ">", "D", "p", "*", "+"};
        std::vector<double> marker_sizes = {2.0, 4.0, 6.0, 8.0};
        
        // Font settings
        std::string font_family = "Arial";
        int font_size = 10;
        int title_size = 14;
        int label_size = 12;
        int tick_size = 8;
        
        // Alpha values
        double fill_alpha = 0.3;
        double line_alpha = 1.0;
        double marker_alpha = 1.0;
    };

    PlotScheme(SchemeType type = SchemeType::DEFAULT);
    virtual ~PlotScheme() = default;

    // Scheme management
    void set_scheme(SchemeType type);
    void load_custom_scheme(const Colors& colors, const Styles& styles);
    void save_scheme(const std::string& filename) const;
    void load_scheme(const std::string& filename);

    // Color access
    const Colors& get_colors() const { return colors_; }
    const Styles& get_styles() const { return styles_; }
    
    // Individual color getters
    std::string get_line_color(int index) const;
    std::string get_line_style(int index) const;
    int get_line_width(int index) const;
    std::string get_marker(int index) const;
    double get_marker_size(int index) const;

    // Color utilities
    std::string lighten_color(const std::string& color, double factor = 0.3) const;
    std::string darken_color(const std::string& color, double factor = 0.3) const;
    std::string add_alpha(const std::string& color, double alpha) const;
    
    // Validation
    bool is_valid_color(const std::string& color) const;
    static std::vector<std::string> get_available_schemes();

private:
    SchemeType type_;
    Colors colors_;
    Styles styles_;
    
    // Scheme initialization
    void init_default_scheme();
    void init_dark_scheme();
    void init_light_scheme();
    void init_professional_scheme();
    void init_colorblind_friendly_scheme();
    
    // Color utilities
    std::string hex_to_rgb(const std::string& hex) const;
    std::string rgb_to_hex(int r, int g, int b) const;
    std::tuple<int, int, int> parse_color(const std::string& color) const;
    std::string blend_colors(const std::string& color1, const std::string& color2, double ratio) const;
};

/**
 * ThemeManager - Manages multiple plot themes and their application
 */
class ThemeManager {
public:
    ThemeManager();
    virtual ~ThemeManager() = default;

    // Theme management
    void register_theme(const std::string& name, const PlotScheme& scheme);
    void set_active_theme(const std::string& name);
    PlotScheme get_theme(const std::string& name) const;
    PlotScheme get_active_theme() const;
    
    // Built-in themes
    void load_builtin_themes();
    std::vector<std::string> get_available_themes() const;
    
    // Theme customization
    void customize_theme(const std::string& base_theme, 
                        const std::string& new_name,
                        const std::map<std::string, std::string>& color_overrides);

private:
    std::map<std::string, PlotScheme> themes_;
    std::string active_theme_ = "default";
    
    void create_default_themes();
};

} // namespace plot
} // namespace backtrader