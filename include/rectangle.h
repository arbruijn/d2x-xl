#ifndef RECTANGLE_H
#define RECTANGLE_H

//------------------------------------------------------------------------------

class CRectangle {
	public:
		int m_x, m_y, m_w, m_h;

		CRectangle (int x = 0, int y = 0, int w = 0, int h = 0, int t = 0) : m_x (x), m_y (y), m_w (w), m_h (h) {}

		void Setup (int x, int y, int w, int h) {
			m_x = x;
			m_y = y;
			m_w = w;
			m_h = h;
			}	

		inline CRectangle& operator= (CRectangle const other) {
			m_x = other.m_x;
			m_y = other.m_y;
			m_w = other.m_w;
			m_h = other.m_h;
			return *this;
			}

		inline CRectangle& operator+= (CRectangle const other) {
			m_x += other.m_x;
			m_y += other.m_y;
			m_w += other.m_w;
			m_h += other.m_h;
			return *this;
			}

		inline CRectangle& operator-= (CRectangle const other) {
			m_x -= other.m_x;
			m_y -= other.m_y;
			m_w -= other.m_w;
			m_h -= other.m_h;
			return *this;
			}

		inline CRectangle operator+ (CRectangle const other) {
			CRectangle rc = *this;
			rc += other;
			return rc;
			}

		inline CRectangle operator- (CRectangle const other) {
			CRectangle rc = *this;
			rc -= other;
			return rc;
			}

		inline int Left (void) { return m_x; }
		inline int Top (void) { return m_y; }
		inline int Right (void) { return m_x + m_w - 1; }
		inline int Bottom (void) { return m_y + m_h - 1; }
		inline int Width (void) { return m_w; }
		inline int Height (void) { return m_h; }

		inline void SetLeft (int x) { m_x = x; }
		inline void SetTop (int y) { m_y = y; }
		inline void SetWidth (int w) { m_w = w; }
		inline void SetHeight (int h) { m_h = h; }

		inline void GetExtent (int& x, int& y, int& w, int& h) {
			x = Left ();
			y = Top ();
			w = Width ();
			h = Height ();
			}
	};

//------------------------------------------------------------------------------
#endif