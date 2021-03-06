#include <iostream>
#include <sstream>
#include <fstream>
#include <fastuidraw/text/glyph_cache.hpp>
#include <fastuidraw/text/freetype_font.hpp>
#include <fastuidraw/text/glyph_selector.hpp>

#include "sdl_painter_demo.hpp"
#include "PanZoomTracker.hpp"
#include "text_helper.hpp"
#include "cycle_value.hpp"

using namespace fastuidraw;

class painter_glyph_test:public sdl_painter_demo
{
public:
  painter_glyph_test(void);
  ~painter_glyph_test();

protected:

  virtual
  void
  derived_init(int w, int h);

  virtual
  void
  draw_frame(void);

  virtual
  void
  handle_event(const SDL_Event&);

private:

  enum return_code
  create_and_add_font(void);

  void
  ready_glyph_attribute_data(void);

  void
  compute_glyphs_and_positions(GlyphRender renderer,
                               float pixel_size_formatting,
                               std::vector<Glyph> &out_glyphs,
                               std::vector<uint32_t> &character_codes);

  void
  change_glyph_renderer(GlyphRender renderer,
                        const_c_array<Glyph> in_glyphs,
                        std::vector<Glyph> &out_glyphs,
                        const_c_array<uint32_t> character_codes);

  void
  update_cts_params(void);

  enum
    {
      draw_glyph_coverage,
      draw_glyph_curvepair,
      draw_glyph_distance,

      number_draw_modes
    };

  command_line_argument_value<std::string> m_font_path;
  command_line_argument_value<std::string> m_font_style, m_font_family;
  command_line_argument_value<bool> m_font_bold, m_font_italic;
  command_line_argument_value<int> m_coverage_pixel_size;
  command_line_argument_value<int> m_distance_pixel_size;
  command_line_argument_value<float> m_max_distance;
  command_line_argument_value<int> m_curve_pair_pixel_size;
  command_line_argument_value<std::string> m_text;
  command_line_argument_value<bool> m_use_file;
  command_line_argument_value<bool> m_draw_glyph_set;
  command_line_argument_value<float> m_render_pixel_size;
  command_line_argument_value<float> m_change_stroke_width_rate;

  reference_counted_ptr<const FontFreeType> m_font;

  vecN<PainterAttributeData, number_draw_modes> m_draws;
  vecN<std::string, number_draw_modes> m_draw_labels;
  vecN<std::vector<Glyph>, number_draw_modes> m_glyphs;
  std::vector<vec2> m_glyph_positions;

  bool m_use_anisotropic_anti_alias;
  bool m_stroke_glyphs;
  float m_stroke_width;
  unsigned int m_current_drawer;
  PanZoomTrackerSDLEvent m_zoomer;
  simple_time m_draw_timer;
};



/////////////////////////////////////
// painter_glyph_test methods
painter_glyph_test::
painter_glyph_test(void):
  m_font_path("/usr/share/fonts/truetype", "font_path", "Specifies path in which to search for fonts", *this),
  m_font_style("Book", "font_style", "Specifies the font style", *this),
  m_font_family("DejaVu Sans", "font_family", "Specifies the font family name", *this),
  m_font_bold(false, "font_bold", "if true select a bold font", *this),
  m_font_italic(false, "font_italic", "if true select an italic font", *this),
  m_coverage_pixel_size(24, "coverage_pixel_size", "Pixel size at which to create coverage glyphs", *this),
  m_distance_pixel_size(48, "distance_pixel_size", "Pixel size at which to create distance field glyphs", *this),
  m_max_distance(96.0f, "max_distance",
                 "value to use for max distance in 64'ths of a pixel "
                 "when generating distance field glyphs", *this),
  m_curve_pair_pixel_size(48, "curvepair_pixel_size", "Pixel size at which to create distance curve pair glyphs", *this),
  m_text("Hello World!", "text", "text to draw to the screen", *this),
  m_use_file(false, "use_file", "if true the value for text gives a filename to display", *this),
  m_draw_glyph_set(false, "draw_glyph_set", "if true, display all glyphs of font instead of text", *this),
  m_render_pixel_size(24.0f, "render_pixel_size", "pixel size at which to display glyphs", *this),
  m_change_stroke_width_rate(10.0f, "change_stroke_width_rate",
                             "rate of change in pixels/sec for changing stroke width "
                             "when changing stroke when key is down",
                             *this),
  m_use_anisotropic_anti_alias(false),
  m_stroke_glyphs(false),
  m_stroke_width(1.0f),
  m_current_drawer(draw_glyph_curvepair)
{
  std::cout << "Controls:\n"
            << "\td:cycle drawing mode: draw coverage glyph, draw distance glyphs "
            << "[hold shift, control or mode to reverse cycle]\n"
            << "\ta:Toggle using anistropic anti-alias glyph rendering\n"
            << "\td:Cycle though text renderer\n"
            << "\tz:reset zoom factor to 1.0\n"
            << "\ts: toggle stroking glyph path\n"
            << "\t[: decrease stroke width(hold left-shift for slower rate and right shift for faster)\n"
            << "\t]: increase stroke width(hold left-shift for slower rate and right shift for faster)\n"
            << "\tMouse Drag (left button): pan\n"
            << "\tHold Mouse (left button), then drag up/down: zoom out/in\n";
}

painter_glyph_test::
~painter_glyph_test()
{
}


enum return_code
painter_glyph_test::
create_and_add_font(void)
{
  FontProperties props;
  props.style(m_font_style.m_value.c_str());
  props.family(m_font_family.m_value.c_str());
  props.bold(m_font_bold.m_value);
  props.italic(m_font_italic.m_value);

  add_fonts_from_path(m_font_path.m_value, m_ft_lib, m_glyph_selector,
                      FontFreeType::RenderParams()
                      .distance_field_max_distance(m_max_distance.m_value)
                      .distance_field_pixel_size(m_distance_pixel_size.m_value)
                      .curve_pair_pixel_size(m_curve_pair_pixel_size.m_value));

  reference_counted_ptr<const FontBase> font;

  font = m_glyph_selector->fetch_font(props);
  std::cout << "Chose font:" << font->properties() << "\n";

  m_font = font.dynamic_cast_ptr<const FontFreeType>();

  return routine_success;
}


void
painter_glyph_test::
derived_init(int w, int h)
{
  FASTUIDRAWunused(w);
  FASTUIDRAWunused(h);

  if(create_and_add_font() == routine_fail)
    {
      end_demo(-1);
      return;
    }

  //put into unit of per ms
  m_change_stroke_width_rate.m_value /= 1000.0f;

  ready_glyph_attribute_data();
  m_draw_timer.restart();
}


void
painter_glyph_test::
change_glyph_renderer(GlyphRender renderer,
                      const_c_array<Glyph> in_glyphs,
                      std::vector<Glyph> &out_glyphs,
                      const_c_array<uint32_t> character_codes)
{
  out_glyphs.resize(in_glyphs.size());
  for(unsigned int i = 0; i < in_glyphs.size(); ++i)
    {
      if(in_glyphs[i].valid())
        {
          out_glyphs[i] = m_glyph_selector->fetch_glyph_no_merging(renderer, in_glyphs[i].layout().m_font, character_codes[i]);
        }
    }
}

void
painter_glyph_test::
compute_glyphs_and_positions(fastuidraw::GlyphRender renderer,
                             float pixel_size_formatting,
                             std::vector<Glyph> &out_glyphs,
                             std::vector<uint32_t> &character_codes)
{
  if(m_draw_glyph_set.m_value)
    {
      float max_height(0.0f);
      unsigned int i, endi, glyph_at_start, navigator_chars;
      float scale_factor, div_scale_factor;
      std::list< std::pair<float, std::string> > navigator;
      std::list< std::pair<float, std::string> >::iterator nav_iter;
      float line_length(800);
      FT_ULong character_code;
      FT_UInt  glyph_index;

      switch(renderer.m_type)
        {
        case distance_field_glyph:
          div_scale_factor = m_font->render_params().distance_field_pixel_size();
          break;
        case curve_pair_glyph:
          div_scale_factor = m_font->render_params().curve_pair_pixel_size();
          break;

        default:
          div_scale_factor = renderer.m_pixel_size;
        }

      scale_factor = pixel_size_formatting / div_scale_factor;

      for(character_code = FT_Get_First_Char(m_font->face(), &glyph_index); glyph_index != 0;
          character_code = FT_Get_Next_Char(m_font->face(), character_code, &glyph_index))
        {
          Glyph g;
          g = m_glyph_selector->fetch_glyph_no_merging(renderer, m_font, character_code);

          assert(g.valid());
          assert(g.layout().m_glyph_code == uint32_t(glyph_index));
          max_height = std::max(max_height, g.layout().m_size.x());
          out_glyphs.push_back(g);
          character_codes.push_back(character_code);
        }

      m_glyph_positions.resize(out_glyphs.size());

      vec2 pen(0.0f, scale_factor * max_height);
      for(navigator_chars = 0, i = 0, endi = out_glyphs.size(), glyph_at_start = 0; i < endi; ++i)
        {
          Glyph g;
          float advance;

          g = out_glyphs[i];
          advance = scale_factor * std::max(g.layout().m_advance.x(), g.layout().m_size.x());

          m_glyph_positions[i] = pen;
          pen.x() += advance;
          if(pen.x() >= line_length)
            {
              std::ostringstream desc;
              desc << "[" << std::setw(5) << glyph_at_start << " - "
                   << std::setw(5) << i << "]";
              navigator.push_back(std::make_pair(pen.y(), desc.str()));
              navigator_chars += navigator.back().second.length();

              pen.y() += scale_factor * max_height;
              pen.x() = 0.0f;
              m_glyph_positions[i] = pen;
              pen.x() += advance;
              glyph_at_start = i + 1;
            }
        }

      character_codes.reserve(out_glyphs.size() + navigator_chars);
      m_glyph_positions.reserve(out_glyphs.size() + navigator_chars);
      out_glyphs.reserve(out_glyphs.size() + navigator_chars);

      std::vector<fastuidraw::Glyph> temp_glyphs;
      std::vector<fastuidraw::vec2> temp_positions;
      std::vector<uint32_t> temp_character_codes;
      for(nav_iter = navigator.begin(); nav_iter != navigator.end(); ++nav_iter)
        {
          std::istringstream stream(nav_iter->second);

          temp_glyphs.clear();
          temp_positions.clear();
          temp_character_codes.clear();
          create_formatted_text(stream, renderer, pixel_size_formatting,
                                m_font, m_glyph_selector, temp_glyphs,
                                temp_positions, temp_character_codes);

          assert(temp_glyphs.size() == temp_positions.size());
          for(unsigned int c = 0; c < temp_glyphs.size(); ++c)
            {
              out_glyphs.push_back(temp_glyphs[c]);
              character_codes.push_back(temp_character_codes[c]);
              m_glyph_positions.push_back(vec2(line_length + temp_positions[c].x(), nav_iter->first) );
            }
        }
    }
  else if(m_use_file.m_value)
    {
      std::ifstream istr(m_text.m_value.c_str(), std::ios::binary);
      if(istr)
        {
          create_formatted_text(istr, renderer, pixel_size_formatting,
                                m_font, m_glyph_selector,
                                out_glyphs, m_glyph_positions,
                                character_codes);
        }
    }
  else
    {
      std::istringstream istr(m_text.m_value);
      create_formatted_text(istr, renderer, pixel_size_formatting,
                            m_font, m_glyph_selector,
                            out_glyphs, m_glyph_positions,
                            character_codes);
    }
}



void
painter_glyph_test::
ready_glyph_attribute_data(void)
{
  std::vector<uint32_t> character_codes;

  {
    GlyphRender renderer(m_coverage_pixel_size.m_value);
    compute_glyphs_and_positions(renderer, m_render_pixel_size.m_value,
                                 m_glyphs[draw_glyph_coverage], character_codes);
    m_draws[draw_glyph_coverage].set_data(PainterAttributeDataFillerGlyphs(cast_c_array(m_glyph_positions),
                                                                           cast_c_array(m_glyphs[draw_glyph_coverage]),
                                                                           m_render_pixel_size.m_value));
    m_draw_labels[draw_glyph_coverage] = "draw_glyph_coverage";
  }

  {
    GlyphRender renderer(distance_field_glyph);
    change_glyph_renderer(renderer,
                          cast_c_array(m_glyphs[draw_glyph_coverage]),
                          m_glyphs[draw_glyph_distance],
                          cast_c_array(character_codes));
    m_draws[draw_glyph_distance].set_data(PainterAttributeDataFillerGlyphs(cast_c_array(m_glyph_positions),
                                                                           cast_c_array(m_glyphs[draw_glyph_distance]),
                                                                           m_render_pixel_size.m_value));
    m_draw_labels[draw_glyph_distance] = "draw_glyph_distance";
  }

  {
    GlyphRender renderer(curve_pair_glyph);
    change_glyph_renderer(renderer,
                          cast_c_array(m_glyphs[draw_glyph_coverage]),
                          m_glyphs[draw_glyph_curvepair],
                          cast_c_array(character_codes));
    m_draws[draw_glyph_curvepair].set_data(PainterAttributeDataFillerGlyphs(cast_c_array(m_glyph_positions),
                                                                            cast_c_array(m_glyphs[draw_glyph_curvepair]),
                                                                            m_render_pixel_size.m_value));
    m_draw_labels[draw_glyph_curvepair] = "draw_glyph_curvepair";
  }
}

void
painter_glyph_test::
draw_frame(void)
{
  update_cts_params();

  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  m_painter->begin();
  ivec2 wh(dimensions());
  float3x3 proj(float_orthogonal_projection_params(0, wh.x(), wh.y(), 0)), m;
  m = proj * m_zoomer.transformation().matrix3();
  m_painter->transformation(m);

  PainterBrush brush;
  brush.pen(1.0, 1.0, 1.0, 1.0);
  m_painter->draw_glyphs(PainterData(&brush),
                         m_draws[m_current_drawer],
                         m_use_anisotropic_anti_alias);

  if(m_stroke_glyphs)
    {
      PainterBrush stroke_brush;
      stroke_brush.pen(0.0, 1.0, 1.0, 0.8);

      PainterStrokeParams st;
      st.miter_limit(1.0f);
      st.width(m_stroke_width);

      // reuse stroke and brush parameters across all glyphs
      PainterPackedValue<PainterBrush> pbr;
      pbr = m_painter->packed_value_pool().create_packed_value(stroke_brush);

      PainterPackedValue<PainterItemShaderData> pst;
      pst = m_painter->packed_value_pool().create_packed_value(st);

      for(unsigned int i = 0; i < m_glyphs[m_current_drawer].size(); ++i)
        {
          if(m_glyphs[m_current_drawer][i].valid())
            {
              m_painter->save();
              m_painter->translate(m_glyph_positions[i]);

              //make the scale of the path match how we scaled the text.
              float sc;
              sc = static_cast<float>(m_render_pixel_size.m_value) / static_cast<float>(m_glyphs[m_current_drawer][i].layout().m_pixel_size);
              m_painter->scale(sc);
              m_painter->stroke_path(PainterData(pst, pbr),
                                     m_glyphs[m_current_drawer][i].path(),
                                     true, PainterEnums::flat_caps, PainterEnums::miter_joins,
                                     true);
              m_painter->restore();
            }
        }
    }

  m_painter->end();
}

void
painter_glyph_test::
update_cts_params(void)
{
  const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);
  assert(keyboard_state != NULL);

  float speed;
  speed = static_cast<float>(m_draw_timer.restart());
  speed *= m_change_stroke_width_rate.m_value;
  speed /= m_zoomer.transformation().scale();

  if(keyboard_state[SDL_SCANCODE_LSHIFT])
    {
      speed *= 0.1f;
    }
  if(keyboard_state[SDL_SCANCODE_RSHIFT])
    {
      speed *= 10.0f;
    }

  if(keyboard_state[SDL_SCANCODE_RIGHTBRACKET])
    {
      m_stroke_width += speed;
    }

  if(keyboard_state[SDL_SCANCODE_LEFTBRACKET] && m_stroke_width > 0.0f)
    {
      m_stroke_width -= speed;
      m_stroke_width = fastuidraw::t_max(m_stroke_width, 0.0f);
    }
}

void
painter_glyph_test::
handle_event(const SDL_Event &ev)
{
  m_zoomer.handle_event(ev);
  switch(ev.type)
    {
    case SDL_QUIT:
      end_demo(0);
      break;

    case SDL_WINDOWEVENT:
      if(ev.window.event == SDL_WINDOWEVENT_RESIZED)
        {
          on_resize(ev.window.data1, ev.window.data2);
        }
      break;

    case SDL_KEYUP:
      switch(ev.key.keysym.sym)
        {
        case SDLK_ESCAPE:
          end_demo(0);
          break;

        case SDLK_a:
          m_use_anisotropic_anti_alias = !m_use_anisotropic_anti_alias;
          if(m_use_anisotropic_anti_alias)
            {
              std::cout << "Using Anistropic anti-alias filtering\n";
            }
          else
            {
              std::cout << "Using Istropic anti-alias filtering\n";
            }
          break;

        case SDLK_d:
          cycle_value(m_current_drawer, ev.key.keysym.mod & (KMOD_SHIFT|KMOD_CTRL|KMOD_ALT), number_draw_modes);
          std::cout << "Drawing " << m_draw_labels[m_current_drawer] << " glyphs\n";
          break;

        case SDLK_z:
          {
            vec2 p, fixed_point(dimensions() / 2);
            ScaleTranslate<float> tr;
            tr = m_zoomer.transformation();
            p = fixed_point - (fixed_point - tr.translation()) / tr.scale();
            m_zoomer.transformation(ScaleTranslate<float>(p));
          }
          break;

        case SDLK_s:
          {
            m_stroke_glyphs = !m_stroke_glyphs;
            std::cout << "Set to ";
            if(!m_stroke_glyphs)
              {
                std::cout << "not ";
              }
            std::cout << " stroke glyph paths\n";
          }
          break;
        }
      break;
    }
}

int
main(int argc, char **argv)
{
  painter_glyph_test G;
  return G.main(argc, argv);
}
