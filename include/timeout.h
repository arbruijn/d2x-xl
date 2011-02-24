/*
 * timeout.h
 *
 *  Created on: 23.02.2011
 *      Author: Dietfrid Mali
 */

#ifndef TIMEOUT_H_
#define TIMEOUT_H_

class CTimeout {
	private:
		time_t	m_t0;
		time_t	m_duration;

	public:
		CTimeout (time_t duration = 1000) : m_duration(duration) { Start (); }

		inline void Start (time_t t = -1) { m_t0 = (t < 0) ? SDL_GetTicks () : t; }

		inline time_t Progress (void) { return m_t0 - SDL_GetTicks (); }

		bool Expired (bool bRestart = true) {
			time_t t = SDL_GetTicks ();
			if (t - m_t0 < m_duration)
				return false;
			if (bRestart)
				Start (t);
			else
				m_t0 += m_duration;
			return true;
			}

		void Throttle (void) {
			time_t t = SDL_GetTicks ();
			time_t dt = t - m_t0;
			if (dt < m_duration) {
				G3_SLEEP (m_duration - (int) dt);
				m_t0 += m_duration;
				}
			else
				m_t0 = t;
			}
	};


#endif /* TIMEOUT_H_ */
