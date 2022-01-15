#include "svg.h"
#include <sstream>

namespace svg {

    using namespace std::literals;

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

    // --- Rgb ---

    Rgb::Rgb(uint8_t r, uint8_t g, uint8_t b) : red(r), green(g), blue(b) {
    }

    // --- Rgba ---

    Rgba::Rgba(uint8_t r, uint8_t g, uint8_t b, double opac) : Rgb::Rgb(r, g, b), opacity(opac) {
    }

    // ----- ColorVisitor -----

    void ColorVisitor::operator()(std::monostate) const
    {
        out << "none"s;
    }

    void ColorVisitor::operator()(std::string str) const
    {
        out << str;
    }

    void ColorVisitor::operator()(Rgb rgb) const
    {
        out << "rgb("sv << static_cast<int>(rgb.red)
            << ","sv << static_cast<int>(rgb.green)
            << ","sv << static_cast<int>(rgb.blue) << ")"sv;
    }

    void ColorVisitor::operator()(Rgba rgba) const
    {
        out << "rgba("sv << static_cast<int>(rgba.red)
            << ","sv << static_cast<int>(rgba.green)
            << ","sv << static_cast<int>(rgba.blue)
            << ","sv << rgba.opacity << ")"sv;
    }

    std::ostream& operator<<(std::ostream& out, Color color) {
        std::visit(ColorVisitor{ out }, color);
        return out;
    }

    // ---- ColorVisitor ----

    std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
        using namespace std::literals;
        switch (line_cap)
        {
        case StrokeLineCap::BUTT:
            out << "butt"sv;
            break;
        case StrokeLineCap::ROUND:
            out << "round"sv;
            break;
        case StrokeLineCap::SQUARE:
            out << "square"sv;
            break;
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
        using namespace std::literals;
        switch (line_join) {
        case StrokeLineJoin::ARCS:
            out << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            out << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            out << "round"sv;
            break;
        }
        return out;
    }

    // ---------- Circle ------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // --- Polyline ---

    Polyline& Polyline::AddPoint(Point point) {
        std::ostringstream out;
        out << point.x << ","s << point.y;
        if (!points_.empty()) {
            points_.push_back(' ');
        }
        points_.append(out.str());
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& ctx) const {
        auto& out = ctx.out;
        out << "<polyline points=\""sv
            << points_
            << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    // --- Text ---


    Text& Text::SetPosition(Point pos) {
        pos_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = font_family;
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = font_weight;
        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_ = ScreenSpecChars(data);
        return *this;
    }

    void Text::RenderObject(const RenderContext& ctx) const {
        auto& out = ctx.out;
        out << "<text"sv;
        RenderAttrs(out);
        out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\" "sv
            << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv
            << "font-size=\""sv << font_size_ << "\""sv;
        if (!font_family_.empty()) {
            out << " font-family=\""sv << font_family_ << "\""sv;
        }
        if (!font_weight_.empty()) {
            out << " font-weight=\""sv << font_weight_ << "\""sv;
        }
        out << ">"sv << data_ << "</text>"sv;
    }

    std::string Text::ScreenSpecChars(const std::string& str) {
        std::ostringstream out;
        for (char ch : str) {
            switch (ch)
            {
            case '\"':
                out << "&quot;"s;
                break;
            case '\'':
                out << "&apos;"s;
                break;
            case '<':
                out << "&lt;"s;
                break;
            case '>':
                out << "gt;"s;
                break;
            case '&':
                out << "&amp;"s;
                break;
            default:
                out << ch;
                break;
            }
        }
        return out.str();
    }

    // --- Document ---

    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.push_back(move(obj));
    }

    void Document::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl
            << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
        RenderContext ctx{ out, 0, 1 };
        for (auto& obj : objects_) {
            ctx.RenderIndent();
            obj->Render(ctx);
        }
        out << "</svg>";
    }

}  // namespace svg