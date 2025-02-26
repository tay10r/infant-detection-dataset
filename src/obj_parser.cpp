#include "obj_parser.h"

#include "exceptions.h"
#include "obj_lexer.h"
#include "obj_model.h"

#include <charconv>
#include <sstream>

#include <cstdio>

namespace {

class parser final
{
public:
  parser(const std::string_view& source, obj_model* model)
    : lexer_(source)
    , model_(model)
  {
  }

  auto done() const -> bool { return lexer_.done(); }

  void iterate()
  {
    const auto token = lexer_.scan();

    if (token.data == "v") {
      model_->vertices.emplace_back(parse_array<float, 3>());
    } else if (token.data == "vn") {
      model_->normals.emplace_back(parse_array<float, 3>());
    } else if (token.data == "vt") {
      model_->texcoords.emplace_back(parse_array<float, 2>());
    } else if (token.data == "o") {
      const auto object = lexer_.scan();
      parse_object_name(object.data);
    } else if (token.data == "g") {
      const auto group = lexer_.scan();
      parse_group_name(group.data);
    } else if (token.data == "f") {
      parse_face();
    } else if (token.data == "s") {
      const auto tok = lexer_.scan();
      if (tok.data == "0") {
        smoothing_ = false;
      } else if (tok.data == "1") {
        smoothing_ = true;
      } else {
        std::ostringstream error;
        error << "Unexpected smoothing argument \"" << tok.data << "\".";
        throw obj_error(error.str());
      }
    } else {
      std::ostringstream stream;
      stream << "Unexpected token '" << token.data << "'.";
      throw obj_error(stream.str());
    }
  }

protected:
  template<typename T, size_t Dim>
  auto parse_array() -> std::array<T, Dim>
  {
    std::array<T, Dim> result;

    for (size_t i = 0; i < Dim; i++) {
      const auto token = lexer_.scan();
      const auto& data = token.data;
      T value;
      const auto r = std::from_chars(data.data(), data.data() + data.size(), value);
      if (r.ptr != (data.data() + data.size())) {
        std::ostringstream stream;
        stream << "Failed to parse number '" << data << "'.";
        throw obj_error(stream.str());
      }
      result[i] = value;
    }

    return result;
  }

  static auto match(const std::string_view& name, const std::string_view& keyword) -> bool
  {
    return name.find(keyword) != std::string_view::npos;
  }

  void parse_group_name(const std::string_view& name)
  {
    const auto is_baby_cls = (static_cast<int>(current_class_) & 0x80) != 1;
    if (!is_baby_cls) {
      // We only care about group names for indicating what body part of the infant
      // we are looking at.
      return;
    }

    if (match(name, "head")) {
      current_class_ = obj_class::baby_head;
    } else if (match(name, "index") || match(name, "middle") || match(name, "ring") || match(name, "pinky") ||
               match(name, "hand") || match(name, "thumb")) {
      current_class_ = obj_class::baby_hand;
    } else if (match(name, "eye")) {
      current_class_ = obj_class::baby_eye;
    } else if (match(name, "iris")) {
      current_class_ = obj_class::baby_iris;
    } else if (match(name, "pupil")) {
      current_class_ = obj_class::baby_pupil;
    } else {
      // restore default class
      current_class_ = obj_class::baby;
    }
  }

  void parse_object_name(const std::string_view& name)
  {
    if (name == "baby") {
      current_class_ = obj_class::baby;
    } else if (name == "floor") {
      current_class_ = obj_class::floor;
    } else if (name == "mattress") {
      current_class_ = obj_class::mattress;
    } else if ((name == "wall") || (name == "ceiling")) {
      current_class_ = obj_class::wall;
    } else if (name == "crib") {
      current_class_ = obj_class::crib;
    } else if (name == "crib_bars") {
      current_class_ = obj_class::crib_bars;
    } else if (name == "blanket") {
      current_class_ = obj_class::blanket;
    } else if ((name == "cabinet") || (name == "cabinet_2") || (name == "cabinet_3")) {
      current_class_ = obj_class::cabinet;
    } else if (name == "chair_frame") {
      current_class_ = obj_class::chair;
    } else if (name == "canvas") {
      current_class_ = obj_class::canvas;
    } else if (name == "painting") {
      current_class_ = obj_class::painting;
    } else if (name == "lamp_1_bottom") {
      current_class_ = obj_class::lamp_bottom;
    } else if (name == "lamp_1_top") {
      current_class_ = obj_class::lamp_top;
    } else if ((name == "toy") || (name == "elephant")) {
      current_class_ = obj_class::toy;
    } else if (name == "name_board") {
      current_class_ = obj_class::name_board;
    } else if (name == "name_text") {
      current_class_ = obj_class::name_text;
    } else {
      current_class_ = obj_class::unknown;
    }
  }

  void parse_face_triplet(face& f, size_t i)
  {
    const auto token = lexer_.scan();
    const auto* begin = token.data.data();
    const auto* end = token.data.data() + token.data.size();

    std::array<size_t, 3> attribs{ 0, 0, 0 };

    for (size_t j = 0; j < 3; j++) {
      size_t value = 0;
      const auto r = std::from_chars(begin, end, value);
      if (value < 1) {
        std::ostringstream stream;
        stream << "Face triplet '" << token.data << "' contains invalid value " << value << " for element " << j << ".";
        throw obj_error(stream.str());
      }
      attribs[j] = value - 1;
      if (r.ptr == end) {
        break;
      }
      begin = r.ptr;
      if (*begin == '/') {
      } else {
        std::ostringstream stream;
        stream << "Face triplet '" << token.data << "' contains bad separator.";
        throw obj_error(stream.str());
      }
      begin++;
    }

    f.position[i] = attribs[0];
    f.texcoord[i] = attribs[1];
    f.normal[i] = attribs[2];
  }

  void parse_face()
  {
    // assume triangulated face
    face f;
    for (size_t i = 0; i < 3; i++) {
      parse_face_triplet(f, i);
    }
    model_->faces.emplace_back(f);
    model_->face_classes.emplace_back(current_class_);
  }

private:
  obj_lexer lexer_;

  obj_model* model_;

  bool smoothing_{ false };

  obj_class current_class_{ obj_class::unknown };
};

} // namespace

auto
parse_obj(const std::string_view& source) -> obj_model
{
  obj_model model;

  parser p(source, &model);

  while (!p.done()) {
    p.iterate();
  }

  return model;
}

auto
parse_obj_file(const char* filename) -> obj_model
{
  auto* file = fopen(filename, "r");
  if (!file) {
    std::ostringstream error;
    error << "Failed to open \"" << filename << "\".";
    throw obj_error(error.str());
  }

  fseek(file, 0, SEEK_END);

  const auto size = ftell(file);

  fseek(file, 0, SEEK_SET);

  if (size == -1L) {
    fclose(file);
    std::ostringstream error;
    error << "Failed to get file size of \"" << filename << "\".";
    throw obj_error(error.str());
  }

  std::string data;

  data.resize(size);

  const auto read_size = fread(data.data(), 1, data.size(), file);

  fclose(file);

  data.resize(read_size);

  return parse_obj(data);
}
