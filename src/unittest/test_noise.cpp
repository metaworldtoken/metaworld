/*
metaworld
Copyright (C) 2010-2014 kwolekr, Ryan Kwolek <kwolekr@metaworld.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "test.h"

#include <cmath>
#include "exceptions.h"
#include "noise.h"

class TestNoise : public TestBase {
public:
	TestNoise() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestNoise"; }

	void runTests(IGameDef *gamedef);

	void testNoise2dPoint();
	void testNoise2dBulk();
	void testNoise3dPoint();
	void testNoise3dBulk();
	void testNoiseInvalidParams();

	static const float expected_2d_results[10 * 10];
	static const float expected_3d_results[10 * 10 * 10];
};

static TestNoise g_test_instance;

void TestNoise::runTests(IGameDef *gamedef)
{
	TEST(testNoise2dPoint);
	TEST(testNoise2dBulk);
	TEST(testNoise3dPoint);
	TEST(testNoise3dBulk);
	TEST(testNoiseInvalidParams);
}

////////////////////////////////////////////////////////////////////////////////

void TestNoise::testNoise2dPoint()
{
	NoiseParams np_normal(20, 40, v3f(50, 50, 50), 9,  5, 0.6, 2.0);

	u32 i = 0;
	for (u32 y = 0; y != 10; y++)
	for (u32 x = 0; x != 10; x++, i++) {
		float actual   = NoisePerlin2D(&np_normal, x, y, 1337);
		float expected = expected_2d_results[i];
		UASSERT(std::fabs(actual - expected) <= 0.00001);
	}
}

void TestNoise::testNoise2dBulk()
{
	NoiseParams np_normal(20, 40, v3f(50, 50, 50), 9,  5, 0.6, 2.0);
	Noise noise_normal_2d(&np_normal, 1337, 10, 10);
	float *noisevals = noise_normal_2d.perlinMap2D(0, 0, NULL);

	for (u32 i = 0; i != 10 * 10; i++) {
		float actual   = noisevals[i];
		float expected = expected_2d_results[i];
		UASSERT(std::fabs(actual - expected) <= 0.00001);
	}
}

void TestNoise::testNoise3dPoint()
{
	NoiseParams np_normal(20, 40, v3f(50, 50, 50), 9,  5, 0.6, 2.0);

	u32 i = 0;
	for (u32 z = 0; z != 10; z++)
	for (u32 y = 0; y != 10; y++)
	for (u32 x = 0; x != 10; x++, i++) {
		float actual   = NoisePerlin3D(&np_normal, x, y, z, 1337);
		float expected = expected_3d_results[i];
		UASSERT(std::fabs(actual - expected) <= 0.00001);
	}
}

void TestNoise::testNoise3dBulk()
{
	NoiseParams np_normal(20, 40, v3f(50, 50, 50), 9, 5, 0.6, 2.0);
	Noise noise_normal_3d(&np_normal, 1337, 10, 10, 10);
	float *noisevals = noise_normal_3d.perlinMap3D(0, 0, 0, NULL);

	for (u32 i = 0; i != 10 * 10 * 10; i++) {
		float actual   = noisevals[i];
		float expected = expected_3d_results[i];
		UASSERT(std::fabs(actual - expected) <= 0.00001);
	}
}

void TestNoise::testNoiseInvalidParams()
{
	bool exception_thrown = false;

	try {
		NoiseParams np_highmem(4, 70, v3f(1, 1, 1), 5, 60, 0.7, 10.0);
		Noise noise_highmem_3d(&np_highmem, 1337, 200, 200, 200);
		noise_highmem_3d.perlinMap3D(0, 0, 0, NULL);
	} catch (InvalidNoiseParamsException &) {
		exception_thrown = true;
	}

	UASSERT(exception_thrown);
}

const float TestNoise::expected_2d_results[10 * 10] = {
	19.11726, 18.49626, 16.48476, 15.02135, 14.75713, 16.26008, 17.54822,
	18.06860, 18.57016, 18.48407, 18.49649, 17.89160, 15.94162, 14.54901,
	14.31298, 15.72643, 16.94669, 17.55494, 18.58796, 18.87925, 16.08101,
	15.53764, 13.83844, 12.77139, 12.73648, 13.95632, 14.97904, 15.81829,
	18.37694, 19.73759, 13.19182, 12.71924, 11.34560, 10.78025, 11.18980,
	12.52303, 13.45012, 14.30001, 17.43298, 19.15244, 10.93217, 10.48625,
	 9.30923,  9.18632, 10.16251, 12.11264, 13.19697, 13.80801, 16.39567,
	17.66203, 10.40222,  9.86070,  8.47223,  8.45471, 10.04780, 13.54730,
	15.33709, 15.48503, 16.46177, 16.52508, 10.80333, 10.19045,  8.59420,
	 8.47646, 10.22676, 14.43173, 16.48353, 16.24859, 16.20863, 15.52847,
	11.01179, 10.45209,  8.98678,  8.83986, 10.43004, 14.46054, 16.29387,
	15.73521, 15.01744, 13.85542, 10.55201, 10.33375,  9.85102, 10.07821,
	11.58235, 15.62046, 17.35505, 16.13181, 12.66011,  9.51853, 11.50994,
	11.54074, 11.77989, 12.29790, 13.76139, 17.81982, 19.49008, 17.79470,
	12.34344,  7.78363,
};

const float TestNoise::expected_3d_results[10 * 10 * 10] = {
	19.11726, 18.01059, 16.90392, 15.79725, 16.37154, 17.18597, 18.00040,
	18.33467, 18.50889, 18.68311, 17.85386, 16.90585, 15.95785, 15.00985,
	15.61132, 16.43415, 17.25697, 17.95415, 18.60942, 19.26471, 16.59046,
	15.80112, 15.01178, 14.22244, 14.85110, 15.68232, 16.51355, 17.57361,
	18.70996, 19.84631, 15.32705, 14.69638, 14.06571, 13.43504, 14.09087,
	14.93050, 15.77012, 17.19309, 18.81050, 20.42790, 15.06729, 14.45855,
	13.84981, 13.24107, 14.39364, 15.79782, 17.20201, 18.42640, 19.59085,
	20.75530, 14.95090, 14.34456, 13.73821, 13.13187, 14.84825, 16.89645,
	18.94465, 19.89025, 20.46832, 21.04639, 14.83452, 14.23057, 13.62662,
	13.02267, 15.30287, 17.99508, 20.68730, 21.35411, 21.34580, 21.33748,
	15.39817, 15.03590, 14.67364, 14.31137, 16.78242, 19.65824, 22.53405,
	22.54626, 21.60395, 20.66164, 16.18850, 16.14768, 16.10686, 16.06603,
	18.60362, 21.50956, 24.41549, 23.64784, 21.65566, 19.66349, 16.97884,
	17.25946, 17.54008, 17.82069, 20.42482, 23.36088, 26.29694, 24.74942,
	21.70738, 18.66534, 18.78506, 17.51834, 16.25162, 14.98489, 15.14217,
	15.50287, 15.86357, 16.40597, 17.00895, 17.61193, 18.20160, 16.98795,
	15.77430, 14.56065, 14.85059, 15.35533, 15.86007, 16.63399, 17.49763,
	18.36128, 17.61814, 16.45757, 15.29699, 14.13641, 14.55902, 15.20779,
	15.85657, 16.86200, 17.98632, 19.11064, 17.03468, 15.92718, 14.81968,
	13.71218, 14.26744, 15.06026, 15.85306, 17.09001, 18.47501, 19.86000,
	16.67870, 15.86256, 15.04641, 14.23026, 15.31397, 16.66909, 18.02420,
	18.89042, 19.59369, 20.29695, 16.35522, 15.86447, 15.37372, 14.88297,
	16.55165, 18.52883, 20.50600, 20.91547, 20.80237, 20.68927, 16.03174,
	15.86639, 15.70103, 15.53568, 17.78933, 20.38857, 22.98780, 22.94051,
	22.01105, 21.08159, 16.42434, 16.61407, 16.80381, 16.99355, 19.16133,
	21.61169, 24.06204, 23.65252, 22.28970, 20.92689, 17.05562, 17.61035,
	18.16508, 18.71981, 20.57809, 22.62260, 24.66711, 23.92686, 22.25835,
	20.58984, 17.68691, 18.60663, 19.52635, 20.44607, 21.99486, 23.63352,
	25.27217, 24.20119, 22.22699, 20.25279, 18.45285, 17.02608, 15.59931,
	14.17254, 13.91279, 13.81976, 13.72674, 14.47727, 15.50900, 16.54073,
	18.54934, 17.07005, 15.59076, 14.11146, 14.08987, 14.27651, 14.46316,
	15.31383, 16.38584, 17.45785, 18.64582, 17.11401, 15.58220, 14.05039,
	14.26694, 14.73326, 15.19958, 16.15038, 17.26268, 18.37498, 18.74231,
	17.15798, 15.57364, 13.98932, 14.44402, 15.19001, 15.93600, 16.98694,
	18.13952, 19.29210, 18.29012, 17.26656, 16.24301, 15.21946, 16.23430,
	17.54035, 18.84639, 19.35445, 19.59653, 19.83860, 17.75954, 17.38438,
	17.00923, 16.63407, 18.25505, 20.16120, 22.06734, 21.94068, 21.13642,
	20.33215, 17.22896, 17.50220, 17.77544, 18.04868, 20.27580, 22.78205,
	25.28829, 24.52691, 22.67631, 20.82571, 17.45050, 18.19224, 18.93398,
	19.67573, 21.54024, 23.56514, 25.59004, 24.75878, 22.97546, 21.19213,
	17.92274, 19.07302, 20.22330, 21.37358, 22.55256, 23.73565, 24.91873,
	24.20587, 22.86103, 21.51619, 18.39499, 19.95381, 21.51263, 23.07145,
	23.56490, 23.90615, 24.24741, 23.65296, 22.74660, 21.84024, 18.12065,
	16.53382, 14.94700, 13.36018, 12.68341, 12.13666, 11.58990, 12.54858,
	14.00906, 15.46955, 18.89708, 17.15214, 15.40721, 13.66227, 13.32914,
	13.19769, 13.06625, 13.99367, 15.27405, 16.55443, 19.67351, 17.77046,
	15.86741, 13.96436, 13.97486, 14.25873, 14.54260, 15.43877, 16.53904,
	17.63931, 20.44994, 18.38877, 16.32761, 14.26645, 14.62059, 15.31977,
	16.01895, 16.88387, 17.80403, 18.72419, 19.90153, 18.67057, 17.43962,
	16.20866, 17.15464, 18.41161, 19.66858, 19.81848, 19.59936, 19.38024,
	19.16386, 18.90429, 18.64473, 18.38517, 19.95845, 21.79357, 23.62868,
	22.96589, 21.47046, 19.97503, 18.42618, 19.13802, 19.84985, 20.56168,
	22.76226, 25.17553, 27.58879, 26.11330, 23.34156, 20.56982, 18.47667,
	19.77041, 21.06416, 22.35790, 23.91914, 25.51859, 27.11804, 25.86504,
	23.66121, 21.45738, 18.78986, 20.53570, 22.28153, 24.02736, 24.52704,
	24.84869, 25.17035, 24.48488, 23.46371, 22.44254, 19.10306, 21.30098,
	23.49890, 25.69682, 25.13494, 24.17879, 23.22265, 23.10473, 23.26621,
	23.42769, 17.93453, 16.72707, 15.51962, 14.31216, 12.96039, 11.58800,
	10.21561, 11.29675, 13.19573, 15.09471, 18.05853, 16.85308, 15.64762,
	14.44216, 13.72634, 13.08047, 12.43459, 13.48912, 15.11045, 16.73179,
	18.18253, 16.97908, 15.77562, 14.57217, 14.49229, 14.57293, 14.65357,
	15.68150, 17.02518, 18.36887, 18.30654, 17.10508, 15.90363, 14.70217,
	15.25825, 16.06540, 16.87255, 17.87387, 18.93991, 20.00595, 17.54117,
	17.32369, 17.10622, 16.88875, 18.07494, 19.46166, 20.84837, 21.12988,
	21.04298, 20.95609, 16.64874, 17.55554, 18.46234, 19.36913, 21.18461,
	23.12989, 25.07517, 24.53784, 23.17297, 21.80810, 15.75632, 17.78738,
	19.81845, 21.84951, 24.29427, 26.79812, 29.30198, 27.94580, 25.30295,
	22.66010, 15.98046, 18.43027, 20.88008, 23.32989, 25.21976, 27.02964,
	28.83951, 27.75863, 25.71416, 23.66970, 16.57679, 19.21017, 21.84355,
	24.47693, 25.41719, 26.11557, 26.81396, 26.37308, 25.55245, 24.73182,
	17.17313, 19.99008, 22.80702, 25.62397, 25.61462, 25.20151, 24.78840,
	24.98753, 25.39074, 25.79395, 17.76927, 17.01824, 16.26722, 15.51620,
	13.45256, 11.20141,  8.95025, 10.14162, 12.48049, 14.81936, 17.05051,
	16.49955, 15.94860, 15.39764, 14.28896, 13.10061, 11.91225, 13.10109,
	15.08232, 17.06355, 16.33175, 15.98086, 15.62998, 15.27909, 15.12537,
	14.99981, 14.87425, 16.06056, 17.68415, 19.30775, 15.61299, 15.46217,
	15.31136, 15.16054, 15.96177, 16.89901, 17.83625, 19.02003, 20.28599,
	21.55194, 14.61341, 15.58383, 16.55426, 17.52469, 18.99524, 20.53725,
	22.07925, 22.56233, 22.69243, 22.82254, 13.57371, 15.79697, 18.02024,
	20.24351, 22.34258, 24.42392, 26.50526, 26.18790, 25.07097, 23.95404,
	12.53401, 16.01011, 19.48622, 22.96232, 25.68993, 28.31060, 30.93126,
	29.81347, 27.44951, 25.08555, 12.98106, 16.67323, 20.36540, 24.05756,
	26.36633, 28.47748, 30.58862, 29.76471, 27.96244, 26.16016, 13.92370,
	17.48634, 21.04897, 24.61161, 26.15244, 27.40443, 28.65643, 28.49117,
	27.85349, 27.21581, 14.86633, 18.29944, 21.73255, 25.16566, 25.93854,
	26.33138, 26.72423, 27.21763, 27.74455, 28.27147, 17.60401, 17.30942,
	17.01482, 16.72023, 13.94473, 10.81481,  7.68490,  8.98648, 11.76524,
	14.54400, 16.04249, 16.14603, 16.24958, 16.35312, 14.85158, 13.12075,
	11.38991, 12.71305, 15.05418, 17.39531, 14.48097, 14.98265, 15.48433,
	15.98602, 15.75844, 15.42668, 15.09493, 16.43962, 18.34312, 20.24663,
	12.91945, 13.81927, 14.71909, 15.61891, 16.66530, 17.73262, 18.79995,
	20.16619, 21.63206, 23.09794, 11.68565, 13.84398, 16.00230, 18.16062,
	19.91554, 21.61284, 23.31013, 23.99478, 24.34188, 24.68898, 10.49868,
	14.03841, 17.57814, 21.11788, 23.50056, 25.71795, 27.93534, 27.83796,
	26.96897, 26.09999,  9.31170, 14.23284, 19.15399, 24.07513, 27.08558,
	29.82307, 32.56055, 31.68113, 29.59606, 27.51099,  9.98166, 14.91619,
	19.85071, 24.78524, 27.51291, 29.92532, 32.33773, 31.77077, 30.21070,
	28.65063, 11.27060, 15.76250, 20.25440, 24.74629, 26.88768, 28.69329,
	30.49889, 30.60925, 30.15453, 29.69981, 12.55955, 16.60881, 20.65808,
	24.70735, 26.26245, 27.46126, 28.66005, 29.44773, 30.09835, 30.74898,
	15.20134, 15.53016, 15.85898, 16.18780, 13.53087, 10.44740,  7.36393,
	 8.95806, 12.11139, 15.26472, 13.87432, 14.52378, 15.17325, 15.82272,
	14.49093, 12.87611, 11.26130, 12.73342, 15.23453, 17.73563, 12.54730,
	13.51741, 14.48752, 15.45763, 15.45100, 15.30483, 15.15867, 16.50878,
	18.35766, 20.20654, 11.22027, 12.51103, 13.80179, 15.09254, 16.41106,
	17.73355, 19.05603, 20.28415, 21.48080, 22.67745, 10.27070, 12.53633,
	14.80195, 17.06758, 19.04654, 20.98454, 22.92254, 23.63840, 23.94687,
	24.25534,  9.37505, 12.70901, 16.04297, 19.37693, 21.92136, 24.35300,
	26.78465, 26.93249, 26.31907, 25.70565,  8.47939, 12.88168, 17.28398,
	21.68627, 24.79618, 27.72146, 30.64674, 30.22658, 28.69127, 27.15597,
	 9.77979, 13.97583, 18.17186, 22.36790, 25.18828, 27.81215, 30.43601,
	30.34293, 29.34420, 28.34548, 11.81220, 15.37712, 18.94204, 22.50695,
	24.75282, 26.81024, 28.86766, 29.40003, 29.42404, 29.44806, 13.84461,
	16.77841, 19.71221, 22.64601, 24.31735, 25.80833, 27.29932, 28.45713,
	29.50388, 30.55064, 12.05287, 13.06077, 14.06866, 15.07656, 12.81500,
	10.08638,  7.35776,  9.30520, 12.81134, 16.31747, 11.31943, 12.47863,
	13.63782, 14.79702, 13.82253, 12.54323, 11.26392, 12.88993, 15.48436,
	18.07880, 10.58600, 11.89649, 13.20698, 14.51747, 14.83005, 15.00007,
	15.17009, 16.47465, 18.15739, 19.84013,  9.85256, 11.31435, 12.77614,
	14.23793, 15.83757, 17.45691, 19.07625, 20.05937, 20.83042, 21.60147,
	 9.36002, 11.37275, 13.38548, 15.39822, 17.58109, 19.78828, 21.99546,
	22.68573, 22.87036, 23.05500,  8.90189, 11.52266, 14.14343, 16.76420,
	19.42976, 22.10172, 24.77368, 25.17519, 24.81987, 24.46455,  8.44375,
	11.67256, 14.90137, 18.13018, 21.27843, 24.41516, 27.55190, 27.66464,
	26.76937, 25.87411, 10.51042, 13.30769, 16.10496, 18.90222, 21.70659,
	24.51197, 27.31734, 27.77045, 27.43945, 27.10846, 13.41869, 15.43789,
	17.45709, 19.47628, 21.66124, 23.86989, 26.07853, 27.08170, 27.68305,
	28.28440, 16.32697, 17.56809, 18.80922, 20.05033, 21.61590, 23.22781,
	24.83972, 26.39296, 27.92665, 29.46033,  8.90439, 10.59137, 12.27835,
	13.96532, 12.09914,  9.72536,  7.35159,  9.65235, 13.51128, 17.37022,
	 8.76455, 10.43347, 12.10239, 13.77132, 13.15412, 12.21033, 11.26655,
	13.04643, 15.73420, 18.42198,  8.62470, 10.27557, 11.92644, 13.57731,
	14.20910, 14.69531, 15.18151, 16.44051, 17.95712, 19.47373,  8.48485,
	10.11767, 11.75049, 13.38331, 15.26408, 17.18027, 19.09647, 19.83460,
	20.18004, 20.52548,  8.44933, 10.20917, 11.96901, 13.72885, 16.11565,
	18.59202, 21.06838, 21.73307, 21.79386, 21.85465,  8.42872, 10.33631,
	12.24389, 14.15147, 16.93816, 19.85044, 22.76272, 23.41788, 23.32067,
	23.22346,  8.40812, 10.46344, 12.51877, 14.57409, 17.76068, 21.10886,
	24.45705, 25.10269, 24.84748, 24.59226, 11.24106, 12.63955, 14.03805,
	15.43654, 18.22489, 21.21178, 24.19868, 25.19796, 25.53469, 25.87143,
	15.02519, 15.49866, 15.97213, 16.44560, 18.56967, 20.92953, 23.28940,
	24.76337, 25.94205, 27.12073, 18.80933, 18.35777, 17.90622, 17.45466,
	18.91445, 20.64729, 22.38013, 24.32880, 26.34941, 28.37003,
};
