#ifndef _RADAR_H
#define _RADAR_H

#define RADAR_SLICES	64
#define BLIP_SLICES	32

class CRadar {
	private:
		static int32_t				radarRanges [5];
		static float			radarSizes [3];
		static float			sizeOffsets [2][3];

		static CAngleVector	aRadar;
		static CFixMatrix		mRadar;
		static float			yOffs [2][CM_LETTERBOX + 1];
		static float			yRadar;
		static float			fRadius;
		static float			fLineWidth;

		static tSinCosf		sinCosRadar [RADAR_SLICES];
		static tSinCosf		sinCosBlip [BLIP_SLICES];
		static int32_t				bInitSinCos;

		static CFloatVector3		shipColors [8];
		static CFloatVector3		guidebotColor;
		static CFloatVector3		robotColor;
		static CFloatVector3		powerupColor;
		static CFloatVector3		radarColor [2];
		static int32_t				bHaveShipColors;

	private:
		CFloatVector	m_vertices [RADAR_SLICES];
		CFixVector		m_vCenter;
		CFloatVector	m_vCenterf;
		float				m_lineWidth;
		CFloatVector	m_offset;
		float				m_radius;

	public:
		CRadar ();

		void Render (void);

	private:
		void ComputeCenter (void);
		void RenderSetup (void);
		void RenderBackground (void);
		void RenderDevice (void);
		void RenderBlip (CObject *objP, float r, float g, float b, float a, int32_t bAbove);
		void RenderObjects (int32_t bAbove);

	};

extern CRadar radar;

#endif /* _RADAR_H */
