#ifndef RECTANGLE_H
#define RECTANGLE_H

//------------------------------------------------------------------------------

class CRectangle {
	public:
		int32_t m_x, m_y, m_w, m_h;

		CRectangle (int32_t x = 0, int32_t y = 0, int32_t w = 0, int32_t h = 0, int32_t t = 0) : m_x (x), m_y (y), m_w (w), m_h (h) {}

		CRectangle (const CRectangle& other) : m_x (other.m_x), m_y (other.m_y), m_w (other.m_w), m_h (other.m_h) {}

		void Setup (int32_t x, int32_t y, int32_t w, int32_t h) {
			m_x = x;
			m_y = y;
			m_w = w;
			m_h = h;
			}	

		inline CRectangle& operator= (const CRectangle other) {
			m_x = other.m_x;
			m_y = other.m_y;
			m_w = other.m_w;
			m_h = other.m_h;
			return *this;
			}

		inline CRectangle& operator+= (const CRectangle other) {
			m_x += other.m_x;
			m_y += other.m_y;
			m_w += other.m_w;
			m_h += other.m_h;
			return *this;
			}

		inline CRectangle& operator-= (const CRectangle other) {
			m_x -= other.m_x;
			m_y -= other.m_y;
			m_w -= other.m_w;
			m_h -= other.m_h;
			return *this;
			}

		inline CRectangle operator+ (const CRectangle other) {
			CRectangle rc = *this;
			rc += other;
			return rc;
			}

		inline CRectangle operator- (const CRectangle other) {
			CRectangle rc = *this;
			rc -= other;
			return rc;
			}

		inline int32_t Left (void) { return m_x; }
		inline int32_t Top (void) { return m_y; }
		inline int32_t Right (void) { return m_x + m_w - 1; }
		inline int32_t Bottom (void) { return m_y + m_h - 1; }
		inline int32_t Width (void) { return m_w; }
		inline int32_t Height (void) { return m_h; }

		inline void SetLeft (int32_t x) { m_x = x; }
		inline void SetTop (int32_t y) { m_y = y; }
		inline void SetWidth (int32_t w) { m_w = w; }
		inline void SetHeight (int32_t h) { m_h = h; }

		inline void GetExtent (int32_t& x, int32_t& y, int32_t& w, int32_t& h) {
			x = Left ();
			y = Top ();
			w = Width ();
			h = Height ();
			}
	};

//------------------------------------------------------------------------------
#endif