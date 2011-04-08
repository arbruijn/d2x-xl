#ifndef _RADAR_H
#define _RADAR_H

#define RADAR_RANGE	radarRanges [gameOpts->render.automap.nRange]
#define RADAR_SLICES	64
#define BLIP_SLICES	32

class CRadar {
	private:
		static int				radarRanges [4];

		static CAngleVector	aRadar;
		static CFixMatrix		mRadar;
		static float			yOffs [2][CM_LETTERBOX + 1];
		static float			yRadar;
		static float			fRadius;
		static float			fLineWidth;

		static tSinCosf		sinCosRadar [RADAR_SLICES];
		static tSinCosf		sinCosBlip [BLIP_SLICES];
		static int				bInitSinCos;

		static tRgbColorf		shipColors [8];
		static tRgbColorf		guidebotColor;
		static tRgbColorf		robotColor;
		static tRgbColorf		powerupColor;
		static tRgbColorf		radarColor [2];
		static int				bHaveShipColors;

	private:
		CFixMatrix		m_mRadar;
		CFixVector		m_vCenter;
		float				m_lineWidth;
		CFloatVector	m_offset;
		float				m_radius;

	public:
		CRadar () : m_lineWidth (1.0f) {}

		void Render (void);

	private:
		void InitShipColors (void);
		void RenderDevice (bool bBackground);
		void RenderBlip (CObject *objP, float r, float g, float b, float a, int bAbove);
		void RenderObjects (int bAbove);

	};

extern CRadar radar;

#endif /* _RADAR_H */
