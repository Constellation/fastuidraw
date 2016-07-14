#pragma once

#include <fastuidraw/painter/painter_packed_value.hpp>

namespace fastuidraw
{

/*!\addtogroup Painter
  @{
 */

  /*!
    A PainterData provides the data for how for a
    Painter to draw content.
   */
  class PainterData
  {
  public:
    /*!
      Holds both a PainterPackedValue and a pointer to a value.
      If m_packed_value is valid, then its value is used. If
      it is NULL and m_value is non-NULL, then the value
      behind m_value is used. Otherwise a value made from
      default ctor of T is used.
     */
    template<typename T>
    class value
    {
    public:
      /*!
        Ctor from a value value.
        \param p value with which to initialize m_value
       */
      value(const T *p = NULL):
        m_value(p)
      {}

      /*!
        Ctor from a packed value.
        \param p value with which to initialize m_packed_value
       */
      value(const PainterPackedValue<T> &p):
        m_value(NULL),
        m_packed_value(p)
      {}

      /*!
        Pointer to value.
       */
      const T *m_value;

      /*!
        Value pre-packed and ready for reuse.
       */
      PainterPackedValue<T> m_packed_value;
    };

    PainterData(void)
    {}

    template<typename T1>
    PainterData(const T1 &r1)
    {
      set(r1);
    }

    template<typename T1, typename T2>
    PainterData(const T1 &r1, const T2 &r2)
    {
      set(r1);
      set(r2);
    }

    template<typename T1, typename T2, typename T3>
    PainterData(const T1 &r1, const T2 &r2, const T3 &r3)
    {
      set(r1);
      set(r2);
      set(r3);
    }

    /*!
      value for brush
     */
    value<PainterBrush> m_brush;

    /*!
      value for vertex shader data
    */
    value<PainterVertexShaderData> m_vertex_shader_data;

    /*!
      value for fragment shader data
    */
    value<PainterFragmentShaderData> m_fragment_shader_data;

    /*!
      Sets m_brush
     */
    PainterData&
    set(const value<PainterBrush> &value)
    {
      m_brush = value;
      return *this;
    }

    /*!
      Sets m_vertex_shader_data
     */
    PainterData&
    set(const value<PainterVertexShaderData> &value)
    {
      m_vertex_shader_data = value;
      return *this;
    }

    /*!
      Sets m_fragment_shader_data
     */
    PainterData&
    set(const value<PainterFragmentShaderData> &value)
    {
      m_fragment_shader_data = value;
      return *this;
    }
  };

/*! @} */
}