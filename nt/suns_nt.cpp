// "Suns" for the Expert Sleepers disting NT.
//
// A display-only algorithm: three suns wander slowly across the sky in the
// upper part of the screen, setting behind the horizon now and then. Below
// the horizon, their light falls through an imagined opening onto the floor,
// merged into one soft pool that drifts and changes shape as the suns move.
//
// It does not touch any audio or CV. Select it in a preset and leave it on
// screen as a "sun page". One parameter: Speed.
//
// Screen: 256 x 64, 16 grey levels, y = 0 at the top.

#include <math.h>
#include <string.h>
#include <new>
#include <distingnt/api.h>

#define SCREEN_W 256
#define SCREEN_H 64
#define HORIZON 42            // y of the horizon line
#define SPRITE 28             // half-size of the area drawn around each sun
#define SPRITE_W (2 * SPRITE + 1)
#define TWO_PI 6.28318530718f

struct SunPath
{
	float period;    // seconds per loop
	float ax, ay;    // amplitude (pixels)
	float yRatio;    // vertical frequency relative to horizontal
	float offset;    // phase offset (0..1)
	float radius;    // disc radius (pixels)
	float rayPeriod; // seconds per ray rotation (negative = other way)
};

static const SunPath kPaths[3] = {
	{ 150.0f,  92.0f,  9.0f, 2.0f, 0.00f, 6.5f,  60.0f },
	{ 190.0f,  80.0f, 11.0f, 3.0f, 0.33f, 4.8f, -75.0f },
	{ 235.0f, 104.0f,  8.0f, 2.0f, 0.66f, 3.6f,  45.0f },
};

// Large buffers live in DRAM (plentiful); the algorithm object stays small.
struct SunsBuffers
{
	float dist[SPRITE_W * SPRITE_W];    // sprite offset -> distance from centre
	float angle[SPRITE_W * SPRITE_W];   // sprite offset -> angle
	uint8_t canvas[SCREEN_W * SCREEN_H];
};

struct _sunsAlgorithm : public _NT_algorithm
{
	_sunsAlgorithm() {}
	~_sunsAlgorithm() {}

	float time;            // seconds, advanced by step()
	SunsBuffers* buf;
};

enum { kParamSpeed };

static const _NT_parameter parameters[] = {
	{ .name = "Speed", .min = 10, .max = 400, .def = 100, .unit = kNT_unitPercent, .scaling = 0, .enumStrings = NULL },
};

static const uint8_t page1[] = { kParamSpeed };
static const _NT_parameterPage pages[] = {
	{ .name = "Suns", .numParams = ARRAY_SIZE(page1), .params = page1 },
};
static const _NT_parameterPages parameterPages = {
	.numPages = ARRAY_SIZE(pages),
	.pages = pages,
};

void calculateRequirements( _NT_algorithmRequirements& req, const int32_t* specifications )
{
	req.numParameters = ARRAY_SIZE(parameters);
	req.sram = sizeof(_sunsAlgorithm);
	req.dram = sizeof(SunsBuffers);
	req.dtc = 0;
	req.itc = 0;
}

_NT_algorithm* construct( const _NT_algorithmMemoryPtrs& ptrs, const _NT_algorithmRequirements& req, const int32_t* specifications )
{
	_sunsAlgorithm* alg = new (ptrs.sram) _sunsAlgorithm();
	alg->parameters = parameters;
	alg->parameterPages = &parameterPages;
	alg->time = 0.0f;
	alg->buf = new (ptrs.dram) SunsBuffers();
	for ( int j = 0; j < SPRITE_W; ++j )
	{
		for ( int i = 0; i < SPRITE_W; ++i )
		{
			float dx = (float)( i - SPRITE );
			float dy = (float)( j - SPRITE );
			alg->buf->dist[ j * SPRITE_W + i ] = sqrtf( dx * dx + dy * dy );
			alg->buf->angle[ j * SPRITE_W + i ] = atan2f( dy, dx );
		}
	}
	return alg;
}

// Audio is untouched; step() only keeps time (sample-accurate, independent
// of how often the screen is redrawn).
void step( _NT_algorithm* self, float* busFrames, int numFramesBy4 )
{
	_sunsAlgorithm* pThis = (_sunsAlgorithm*)self;
	float speed = pThis->v[kParamSpeed] * 0.01f;
	pThis->time += speed * (float)( numFramesBy4 * 4 ) / (float)NT_globals.sampleRate;
	if ( pThis->time > 24.0f * 3600.0f )
		pThis->time = 0.0f;
}

static void drawSun( _sunsAlgorithm* a, float cx, float cy, float r, float phase )
{
	int icx = (int)( cx + 0.5f );
	int icy = (int)( cy + 0.5f );
	for ( int j = 0; j < SPRITE_W; ++j )
	{
		int y = icy + j - SPRITE;
		if ( y < 0 || y >= HORIZON )   // the sun sets behind the horizon
			continue;
		for ( int i = 0; i < SPRITE_W; ++i )
		{
			int x = icx + i - SPRITE;
			if ( x < 0 || x >= SCREEN_W )
				continue;
			int k = j * SPRITE_W + i;
			float d = a->buf->dist[k];
			if ( d > r * 4.0f )
				continue;
			float v;
			if ( d <= r )
				v = 1.0f;
			else
			{
				v = 0.35f * expf( -( d - r ) / ( r * 0.6f ) );
				float c = cosf( 8.0f * a->buf->angle[k] + phase );
				if ( c > 0.0f )
				{
					float c2 = c * c;
					float c4 = c2 * c2;
					float rv = c4 * c4 * 0.9f * ( 1.0f - ( d - r ) / ( r * 3.0f ) );
					if ( rv > v )
						v = rv;
				}
			}
			if ( v <= 0.0f )
				continue;
			int colour = v >= 1.0f ? 15 : (int)( v * 15.0f + 0.5f );
			uint8_t& px = a->buf->canvas[ y * SCREEN_W + x ];
			if ( colour > px )
				px = (uint8_t)colour;
		}
	}
}

static void drawFloor( _sunsAlgorithm* a, const float* cx, const float* cy, const float* r )
{
	// One pool of light: centred on the brightness-weighted centre of the suns
	// (mirrored, as if the light comes through the screen), wider when the
	// suns are spread apart, nearer the viewer when they are high.
	float w[3], total = 0.0f, mx = 0.0f, my = 0.0f;
	for ( int n = 0; n < 3; ++n )
	{
		w[n] = r[n] * r[n];
		total += w[n];
		mx += w[n] * cx[n];
		my += w[n] * cy[n];
	}
	mx /= total;
	my /= total;
	float varx = 0.0f;
	for ( int n = 0; n < 3; ++n )
	{
		float d = cx[n] - mx;
		varx += w[n] * d * d;
	}
	float poolW = 1.0f + sqrtf( varx / total ) / 60.0f;

	float pcx = 128.0f - 0.84f * ( mx - 128.0f );
	float pcy = 49.0f + ( 21.0f - my ) * 0.6f;   // sun high (small y) -> patch nearer (larger y)
	if ( pcy < HORIZON + 4.0f ) pcy = HORIZON + 4.0f;
	if ( pcy > SCREEN_H - 4.0f ) pcy = SCREEN_H - 4.0f;

	float px[3], py[3];
	for ( int n = 0; n < 3; ++n )
	{
		px[n] = pcx + 0.22f * ( -0.84f * ( cx[n] - mx ) );
		py[n] = pcy + 0.15f * ( cy[n] - my ) * 0.6f;
	}

	const int floorH = SCREEN_H - HORIZON - 1;
	for ( int y = HORIZON + 1; y < SCREEN_H; ++y )
	{
		float depth = (float)( y - HORIZON ) / (float)floorH;   // 0 far .. 1 near
		float spread = 14.0f + 34.0f * depth;
		float ambient = 0.03f + 0.07f * depth;
		for ( int x = 0; x < SCREEN_W; ++x )
		{
			float dx = ( (float)x - pcx ) / ( spread * poolW );
			float dy = ( (float)y - pcy ) / ( 3.0f + 4.0f * depth );
			float e = dx * dx + dy * dy;
			float v = 0.0f;
			if ( e < 9.0f )
				v = 0.92f * expf( -e * e * 0.35f );
			for ( int n = 0; n < 3; ++n )
			{
				float lx = ( (float)x - px[n] ) / ( spread * 0.55f );
				float ly = ( (float)y - py[n] ) / ( 2.5f + 3.0f * depth );
				float le = lx * lx + ly * ly;
				if ( le < 6.0f )
					v += 0.54f * ( w[n] / total ) * expf( -le );
			}
			v += ambient;
			if ( v > 1.0f )
				v = 1.0f;
			int colour = (int)( v * 15.0f + 0.5f );
			if ( colour > 0 )
				a->buf->canvas[ y * SCREEN_W + x ] = (uint8_t)colour;
		}
	}

	// horizon line
	for ( int x = 0; x < SCREEN_W; ++x )
		a->buf->canvas[ HORIZON * SCREEN_W + x ] = 5;
}

bool draw( _NT_algorithm* self )
{
	_sunsAlgorithm* a = (_sunsAlgorithm*)self;
	float t = a->time;

	memset( a->buf->canvas, 0, sizeof(a->buf->canvas) );

	float cx[3], cy[3], r[3];
	for ( int n = 0; n < 3; ++n )
	{
		const SunPath& p = kPaths[n];
		float ang = TWO_PI * ( t / p.period + p.offset );
		cx[n] = 128.0f + p.ax * sinf( ang );
		cy[n] = 21.0f + p.ay * sinf( p.yRatio * ang );
		r[n] = p.radius * ( 1.0f + 0.12f * sinf( TWO_PI * t / ( 7.0f + n ) ) );
		drawSun( a, cx[n], cy[n], r[n], TWO_PI * t / p.rayPeriod );
	}
	drawFloor( a, cx, cy, r );

	// copy to the screen through the documented drawing call
	for ( int y = 0; y < SCREEN_H; ++y )
	{
		const uint8_t* row = &a->buf->canvas[ y * SCREEN_W ];
		for ( int x = 0; x < SCREEN_W; ++x )
		{
			if ( row[x] )
				NT_drawShapeI( kNT_point, x, y, x, y, row[x] );
		}
	}

	return true;   // full screen: hide the standard parameter line
}

static const _NT_factory factory =
{
	.guid = NT_MULTICHAR( 'Z', 's', 'u', 'n' ),
	.name = "Suns",
	.description = "Three suns and their light on the floor (display only)",
	.numSpecifications = 0,
	.calculateRequirements = calculateRequirements,
	.construct = construct,
	.parameterChanged = NULL,
	.step = step,
	.draw = draw,
	.tags = kNT_tagUtility,
};

uintptr_t pluginEntry( _NT_selector selector, uint32_t data )
{
	switch ( selector )
	{
	case kNT_selector_version:
		return kNT_apiVersionCurrent;
	case kNT_selector_numFactories:
		return 1;
	case kNT_selector_factoryInfo:
		return (uintptr_t)( ( data == 0 ) ? &factory : NULL );
	}
	return 0;
}
