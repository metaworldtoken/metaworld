/*
metaworld
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "clientmap.h"
#include "client.h"
#include "mapblock_mesh.h"
#include <IMaterialRenderer.h>
#include <matrix4.h>
#include "mapsector.h"
#include "mapblock.h"
#include "profiler.h"
#include "settings.h"
#include "camera.h"               // CameraModes
#include "util/basic_macros.h"
#include <algorithm>
#include "client/renderingengine.h"

// struct MeshBufListList
void MeshBufListList::clear()
{
	for (auto &list : lists)
		list.clear();
}

void MeshBufListList::add(scene::IMeshBuffer *buf, v3s16 position, u8 layer)
{
	// Append to the correct layer
	std::vector<MeshBufList> &list = lists[layer];
	const video::SMaterial &m = buf->getMaterial();
	for (MeshBufList &l : list) {
		// comparing a full material is quite expensive so we don't do it if
		// not even first texture is equal
		if (l.m.TextureLayer[0].Texture != m.TextureLayer[0].Texture)
			continue;

		if (l.m == m) {
			l.bufs.emplace_back(position, buf);
			return;
		}
	}
	MeshBufList l;
	l.m = m;
	l.bufs.emplace_back(position, buf);
	list.emplace_back(l);
}

// ClientMap

ClientMap::ClientMap(
		Client *client,
		RenderingEngine *rendering_engine,
		MapDrawControl &control,
		s32 id
):
	Map(client),
	scene::ISceneNode(rendering_engine->get_scene_manager()->getRootSceneNode(),
		rendering_engine->get_scene_manager(), id),
	m_client(client),
	m_rendering_engine(rendering_engine),
	m_control(control),
	m_drawlist(MapBlockComparer(v3s16(0,0,0)))
{

	/*
	 * @Liso: Sadly C++ doesn't have introspection, so the only way we have to know
	 * the class is whith a name ;) Name property cames from ISceneNode base class.
	 */
	Name = "ClientMap";
	m_box = aabb3f(-BS*1000000,-BS*1000000,-BS*1000000,
			BS*1000000,BS*1000000,BS*1000000);

	/* TODO: Add a callback function so these can be updated when a setting
	 *       changes.  At this point in time it doesn't matter (e.g. /set
	 *       is documented to change server settings only)
	 *
	 * TODO: Local caching of settings is not optimal and should at some stage
	 *       be updated to use a global settings object for getting thse values
	 *       (as opposed to the this local caching). This can be addressed in
	 *       a later release.
	 */
	m_cache_trilinear_filter  = g_settings->getBool("trilinear_filter");
	m_cache_bilinear_filter   = g_settings->getBool("bilinear_filter");
	m_cache_anistropic_filter = g_settings->getBool("anisotropic_filter");

}

MapSector * ClientMap::emergeSector(v2s16 p2d)
{
	// Check that it doesn't exist already
	MapSector *sector = getSectorNoGenerate(p2d);

	// Create it if it does not exist yet
	if (!sector) {
		sector = new MapSector(this, p2d, m_gamedef);
		m_sectors[p2d] = sector;
	}

	return sector;
}

void ClientMap::OnRegisterSceneNode()
{
	if(IsVisible)
	{
		SceneManager->registerNodeForRendering(this, scene::ESNRP_SOLID);
		SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);
	}

	ISceneNode::OnRegisterSceneNode();

	if (!m_added_to_shadow_renderer) {
		m_added_to_shadow_renderer = true;
		if (auto shadows = m_rendering_engine->get_shadow_renderer())
			shadows->addNodeToShadowList(this);
	}
}

void ClientMap::getBlocksInViewRange(v3s16 cam_pos_nodes,
		v3s16 *p_blocks_min, v3s16 *p_blocks_max, float range)
{
	if (range <= 0.0f)
		range = m_control.wanted_range;

	v3s16 box_nodes_d = range * v3s16(1, 1, 1);
	// Define p_nodes_min/max as v3s32 because 'cam_pos_nodes -/+ box_nodes_d'
	// can exceed the range of v3s16 when a large view range is used near the
	// world edges.
	v3s32 p_nodes_min(
		cam_pos_nodes.X - box_nodes_d.X,
		cam_pos_nodes.Y - box_nodes_d.Y,
		cam_pos_nodes.Z - box_nodes_d.Z);
	v3s32 p_nodes_max(
		cam_pos_nodes.X + box_nodes_d.X,
		cam_pos_nodes.Y + box_nodes_d.Y,
		cam_pos_nodes.Z + box_nodes_d.Z);
	// Take a fair amount as we will be dropping more out later
	// Umm... these additions are a bit strange but they are needed.
	*p_blocks_min = v3s16(
			p_nodes_min.X / MAP_BLOCKSIZE - 3,
			p_nodes_min.Y / MAP_BLOCKSIZE - 3,
			p_nodes_min.Z / MAP_BLOCKSIZE - 3);
	*p_blocks_max = v3s16(
			p_nodes_max.X / MAP_BLOCKSIZE + 1,
			p_nodes_max.Y / MAP_BLOCKSIZE + 1,
			p_nodes_max.Z / MAP_BLOCKSIZE + 1);
}

void ClientMap::updateDrawList()
{
	ScopeProfiler sp(g_profiler, "CM::updateDrawList()", SPT_AVG);

	m_needs_update_drawlist = false;

	for (auto &i : m_drawlist) {
		MapBlock *block = i.second;
		block->refDrop();
	}
	m_drawlist.clear();

	const v3f camera_position = m_camera_position;
	const v3f camera_direction = m_camera_direction;

	// Use a higher fov to accomodate faster camera movements.
	// Blocks are cropped better when they are drawn.
	const f32 camera_fov = m_camera_fov * 1.1f;

	v3s16 cam_pos_nodes = floatToInt(camera_position, BS);

	v3s16 p_blocks_min;
	v3s16 p_blocks_max;
	getBlocksInViewRange(cam_pos_nodes, &p_blocks_min, &p_blocks_max);

	// Read the vision range, unless unlimited range is enabled.
	float range = m_control.range_all ? 1e7 : m_control.wanted_range;

	// Number of blocks currently loaded by the client
	u32 blocks_loaded = 0;
	// Number of blocks with mesh in rendering range
	u32 blocks_in_range_with_mesh = 0;
	// Number of blocks occlusion culled
	u32 blocks_occlusion_culled = 0;

	// No occlusion culling when free_move is on and camera is
	// inside ground
	bool occlusion_culling_enabled = true;
	if (g_settings->getBool("free_move") && g_settings->getBool("noclip")) {
		MapNode n = getNode(cam_pos_nodes);
		if (n.getContent() == CONTENT_IGNORE ||
				m_nodedef->get(n).solidness == 2)
			occlusion_culling_enabled = false;
	}

	v3s16 camera_block = getContainerPos(cam_pos_nodes, MAP_BLOCKSIZE);
	m_drawlist = std::map<v3s16, MapBlock*, MapBlockComparer>(MapBlockComparer(camera_block));

	// Uncomment to debug occluded blocks in the wireframe mode
	// TODO: Include this as a flag for an extended debugging setting
	//if (occlusion_culling_enabled && m_control.show_wireframe)
	//    occlusion_culling_enabled = porting::getTimeS() & 1;

	for (const auto &sector_it : m_sectors) {
		MapSector *sector = sector_it.second;
		v2s16 sp = sector->getPos();

		blocks_loaded += sector->size();
		if (!m_control.range_all) {
			if (sp.X < p_blocks_min.X || sp.X > p_blocks_max.X ||
					sp.Y < p_blocks_min.Z || sp.Y > p_blocks_max.Z)
				continue;
		}

		MapBlockVect sectorblocks;
		sector->getBlocks(sectorblocks);

		/*
			Loop through blocks in sector
		*/

		u32 sector_blocks_drawn = 0;

		for (MapBlock *block : sectorblocks) {
			/*
				Compare block position to camera position, skip
				if not seen on display
			*/

			if (!block->mesh) {
				// Ignore if mesh doesn't exist
				continue;
			}

			v3s16 block_coord = block->getPos();
			v3s16 block_position = block->getPosRelative() + MAP_BLOCKSIZE / 2;

			// First, perform a simple distance check, with a padding of one extra block.
			if (!m_control.range_all &&
					block_position.getDistanceFrom(cam_pos_nodes) > range + MAP_BLOCKSIZE)
				continue; // Out of range, skip.

			// Keep the block alive as long as it is in range.
			block->resetUsageTimer();
			blocks_in_range_with_mesh++;

			// Frustum culling
			float d = 0.0;
			if (!isBlockInSight(block_coord, camera_position,
					camera_direction, camera_fov, range * BS, &d))
				continue;

			// Occlusion culling
			if ((!m_control.range_all && d > m_control.wanted_range * BS) ||
					(occlusion_culling_enabled && isBlockOccluded(block, cam_pos_nodes))) {
				blocks_occlusion_culled++;
				continue;
			}

			// Add to set
			block->refGrab();
			m_drawlist[block_coord] = block;

			sector_blocks_drawn++;
		} // foreach sectorblocks

		if (sector_blocks_drawn != 0)
			m_last_drawn_sectors.insert(sp);
	}

	g_profiler->avg("MapBlock meshes in range [#]", blocks_in_range_with_mesh);
	g_profiler->avg("MapBlocks occlusion culled [#]", blocks_occlusion_culled);
	g_profiler->avg("MapBlocks drawn [#]", m_drawlist.size());
	g_profiler->avg("MapBlocks loaded [#]", blocks_loaded);
}

void ClientMap::renderMap(video::IVideoDriver* driver, s32 pass)
{
	bool is_transparent_pass = pass == scene::ESNRP_TRANSPARENT;

	std::string prefix;
	if (pass == scene::ESNRP_SOLID)
		prefix = "renderMap(SOLID): ";
	else
		prefix = "renderMap(TRANSPARENT): ";

	/*
		This is called two times per frame, reset on the non-transparent one
	*/
	if (pass == scene::ESNRP_SOLID)
		m_last_drawn_sectors.clear();

	/*
		Get animation parameters
	*/
	const float animation_time = m_client->getAnimationTime();
	const int crack = m_client->getCrackLevel();
	const u32 daynight_ratio = m_client->getEnv().getDayNightRatio();

	const v3f camera_position = m_camera_position;

	/*
		Get all blocks and draw all visible ones
	*/

	u32 vertex_count = 0;
	u32 drawcall_count = 0;

	// For limiting number of mesh animations per frame
	u32 mesh_animate_count = 0;
	//u32 mesh_animate_count_far = 0;

	/*
		Draw the selected MapBlocks
	*/

	MeshBufListList grouped_buffers;

	struct DrawDescriptor {
		v3s16 m_pos;
		scene::IMeshBuffer *m_buffer;
		bool m_reuse_material;

		DrawDescriptor(const v3s16 &pos, scene::IMeshBuffer *buffer, bool reuse_material) :
			m_pos(pos), m_buffer(buffer), m_reuse_material(reuse_material)
		{}
	};

	std::vector<DrawDescriptor> draw_order;
	video::SMaterial previous_material;

	for (auto &i : m_drawlist) {
		v3s16 block_pos = i.first;
		MapBlock *block = i.second;

		// If the mesh of the block happened to get deleted, ignore it
		if (!block->mesh)
			continue;

		v3f block_pos_r = intToFloat(block->getPosRelative() + MAP_BLOCKSIZE / 2, BS);
		float d = camera_position.getDistanceFrom(block_pos_r);
		d = MYMAX(0,d - BLOCK_MAX_RADIUS);

		// Mesh animation
		if (pass == scene::ESNRP_SOLID) {
			MapBlockMesh *mapBlockMesh = block->mesh;
			assert(mapBlockMesh);
			// Pretty random but this should work somewhat nicely
			bool faraway = d >= BS * 50;
			if (mapBlockMesh->isAnimationForced() || !faraway ||
					mesh_animate_count < (m_control.range_all ? 200 : 50)) {

				bool animated = mapBlockMesh->animate(faraway, animation_time,
					crack, daynight_ratio);
				if (animated)
					mesh_animate_count++;
			} else {
				mapBlockMesh->decreaseAnimationForceTimer();
			}
		}

		/*
			Get the meshbuffers of the block
		*/
		{
			MapBlockMesh *mapBlockMesh = block->mesh;
			assert(mapBlockMesh);

			for (int layer = 0; layer < MAX_TILE_LAYERS; layer++) {
				scene::IMesh *mesh = mapBlockMesh->getMesh(layer);
				assert(mesh);

				u32 c = mesh->getMeshBufferCount();
				for (u32 i = 0; i < c; i++) {
					scene::IMeshBuffer *buf = mesh->getMeshBuffer(i);

					video::SMaterial& material = buf->getMaterial();
					video::IMaterialRenderer* rnd =
						driver->getMaterialRenderer(material.MaterialType);
					bool transparent = (rnd && rnd->isTransparent());
					if (transparent == is_transparent_pass) {
						if (buf->getVertexCount() == 0)
							errorstream << "Block [" << analyze_block(block)
								<< "] contains an empty meshbuf" << std::endl;

						material.setFlag(video::EMF_TRILINEAR_FILTER,
							m_cache_trilinear_filter);
						material.setFlag(video::EMF_BILINEAR_FILTER,
							m_cache_bilinear_filter);
						material.setFlag(video::EMF_ANISOTROPIC_FILTER,
							m_cache_anistropic_filter);
						material.setFlag(video::EMF_WIREFRAME,
							m_control.show_wireframe);

						if (is_transparent_pass) {
							// Same comparison as in MeshBufListList
							bool new_material = material.getTexture(0) != previous_material.getTexture(0) ||
									material != previous_material;

							draw_order.emplace_back(block_pos, buf, !new_material);

							if (new_material)
								previous_material = material;
						}
						else {
							grouped_buffers.add(buf, block_pos, layer);
						}
					}
				}
			}
		}
	}

	// Capture draw order for all solid meshes
	for (auto &lists : grouped_buffers.lists) {
		for (MeshBufList &list : lists) {
			// iterate in reverse to draw closest blocks first
			for (auto it = list.bufs.rbegin(); it != list.bufs.rend(); ++it) {
				draw_order.emplace_back(it->first, it->second, it != list.bufs.rbegin());
			}
		}
	}

	TimeTaker draw("Drawing mesh buffers");

	core::matrix4 m; // Model matrix
	v3f offset = intToFloat(m_camera_offset, BS);
	u32 material_swaps = 0;

	// Render all mesh buffers in order
	drawcall_count += draw_order.size();
	for (auto &descriptor : draw_order) {
		scene::IMeshBuffer *buf = descriptor.m_buffer;

		// Check and abort if the machine is swapping a lot
		if (draw.getTimerTime() > 2000) {
			infostream << "ClientMap::renderMap(): Rendering took >2s, " <<
					"returning." << std::endl;
			return;
		}

		if (!descriptor.m_reuse_material) {
			auto &material = buf->getMaterial();
			// pass the shadow map texture to the buffer texture
			ShadowRenderer *shadow = m_rendering_engine->get_shadow_renderer();
			if (shadow && shadow->is_active()) {
				auto &layer = material.TextureLayer[3];
				layer.Texture = shadow->get_texture();
				layer.TextureWrapU = video::E_TEXTURE_CLAMP::ETC_CLAMP_TO_EDGE;
				layer.TextureWrapV = video::E_TEXTURE_CLAMP::ETC_CLAMP_TO_EDGE;
				// Do not enable filter on shadow texture to avoid visual artifacts
				// with colored shadows.
				// Filtering is done in shader code anyway
				layer.TrilinearFilter = false;
			}
			driver->setMaterial(material);
			++material_swaps;
		}

		v3f block_wpos = intToFloat(descriptor.m_pos * MAP_BLOCKSIZE, BS);
		m.setTranslation(block_wpos - offset);

		driver->setTransform(video::ETS_WORLD, m);
		driver->drawMeshBuffer(buf);
		vertex_count += buf->getVertexCount();
	}

	g_profiler->avg(prefix + "draw meshes [ms]", draw.stop(true));

	// Log only on solid pass because values are the same
	if (pass == scene::ESNRP_SOLID) {
		g_profiler->avg("renderMap(): animated meshes [#]", mesh_animate_count);
	}

	if (pass == scene::ESNRP_TRANSPARENT) {
		g_profiler->avg("renderMap(): transparent buffers [#]", draw_order.size());
	}

	g_profiler->avg(prefix + "vertices drawn [#]", vertex_count);
	g_profiler->avg(prefix + "drawcalls [#]", drawcall_count);
	g_profiler->avg(prefix + "material swaps [#]", material_swaps);
}

static bool getVisibleBrightness(Map *map, const v3f &p0, v3f dir, float step,
	float step_multiplier, float start_distance, float end_distance,
	const NodeDefManager *ndef, u32 daylight_factor, float sunlight_min_d,
	int *result, bool *sunlight_seen)
{
	int brightness_sum = 0;
	int brightness_count = 0;
	float distance = start_distance;
	dir.normalize();
	v3f pf = p0;
	pf += dir * distance;
	int noncount = 0;
	bool nonlight_seen = false;
	bool allow_allowing_non_sunlight_propagates = false;
	bool allow_non_sunlight_propagates = false;
	// Check content nearly at camera position
	{
		v3s16 p = floatToInt(p0 /*+ dir * 3*BS*/, BS);
		MapNode n = map->getNode(p);
		if(ndef->get(n).param_type == CPT_LIGHT &&
				!ndef->get(n).sunlight_propagates)
			allow_allowing_non_sunlight_propagates = true;
	}
	// If would start at CONTENT_IGNORE, start closer
	{
		v3s16 p = floatToInt(pf, BS);
		MapNode n = map->getNode(p);
		if(n.getContent() == CONTENT_IGNORE){
			float newd = 2*BS;
			pf = p0 + dir * 2*newd;
			distance = newd;
			sunlight_min_d = 0;
		}
	}
	for (int i=0; distance < end_distance; i++) {
		pf += dir * step;
		distance += step;
		step *= step_multiplier;

		v3s16 p = floatToInt(pf, BS);
		MapNode n = map->getNode(p);
		if (allow_allowing_non_sunlight_propagates && i == 0 &&
				ndef->get(n).param_type == CPT_LIGHT &&
				!ndef->get(n).sunlight_propagates) {
			allow_non_sunlight_propagates = true;
		}

		if (ndef->get(n).param_type != CPT_LIGHT ||
				(!ndef->get(n).sunlight_propagates &&
					!allow_non_sunlight_propagates)){
			nonlight_seen = true;
			noncount++;
			if(noncount >= 4)
				break;
			continue;
		}

		if (distance >= sunlight_min_d && !*sunlight_seen && !nonlight_seen)
			if (n.getLight(LIGHTBANK_DAY, ndef) == LIGHT_SUN)
				*sunlight_seen = true;
		noncount = 0;
		brightness_sum += decode_light(n.getLightBlend(daylight_factor, ndef));
		brightness_count++;
	}
	*result = 0;
	if(brightness_count == 0)
		return false;
	*result = brightness_sum / brightness_count;
	/*std::cerr<<"Sampled "<<brightness_count<<" points; result="
			<<(*result)<<std::endl;*/
	return true;
}

int ClientMap::getBackgroundBrightness(float max_d, u32 daylight_factor,
		int oldvalue, bool *sunlight_seen_result)
{
	ScopeProfiler sp(g_profiler, "CM::getBackgroundBrightness", SPT_AVG);
	static v3f z_directions[50] = {
		v3f(-100, 0, 0)
	};
	static f32 z_offsets[50] = {
		-1000,
	};

	if (z_directions[0].X < -99) {
		for (u32 i = 0; i < ARRLEN(z_directions); i++) {
			// Assumes FOV of 72 and 16/9 aspect ratio
			z_directions[i] = v3f(
				0.02 * myrand_range(-100, 100),
				1.0,
				0.01 * myrand_range(-100, 100)
			).normalize();
			z_offsets[i] = 0.01 * myrand_range(0,100);
		}
	}

	int sunlight_seen_count = 0;
	float sunlight_min_d = max_d*0.8;
	if(sunlight_min_d > 35*BS)
		sunlight_min_d = 35*BS;
	std::vector<int> values;
	values.reserve(ARRLEN(z_directions));
	for (u32 i = 0; i < ARRLEN(z_directions); i++) {
		v3f z_dir = z_directions[i];
		core::CMatrix4<f32> a;
		a.buildRotateFromTo(v3f(0,1,0), z_dir);
		v3f dir = m_camera_direction;
		a.rotateVect(dir);
		int br = 0;
		float step = BS*1.5;
		if(max_d > 35*BS)
			step = max_d / 35 * 1.5;
		float off = step * z_offsets[i];
		bool sunlight_seen_now = false;
		bool ok = getVisibleBrightness(this, m_camera_position, dir,
				step, 1.0, max_d*0.6+off, max_d, m_nodedef, daylight_factor,
				sunlight_min_d,
				&br, &sunlight_seen_now);
		if(sunlight_seen_now)
			sunlight_seen_count++;
		if(!ok)
			continue;
		values.push_back(br);
		// Don't try too much if being in the sun is clear
		if(sunlight_seen_count >= 20)
			break;
	}
	int brightness_sum = 0;
	int brightness_count = 0;
	std::sort(values.begin(), values.end());
	u32 num_values_to_use = values.size();
	if(num_values_to_use >= 10)
		num_values_to_use -= num_values_to_use/2;
	else if(num_values_to_use >= 7)
		num_values_to_use -= num_values_to_use/3;
	u32 first_value_i = (values.size() - num_values_to_use) / 2;

	for (u32 i=first_value_i; i < first_value_i + num_values_to_use; i++) {
		brightness_sum += values[i];
		brightness_count++;
	}

	int ret = 0;
	if(brightness_count == 0){
		MapNode n = getNode(floatToInt(m_camera_position, BS));
		if(m_nodedef->get(n).param_type == CPT_LIGHT){
			ret = decode_light(n.getLightBlend(daylight_factor, m_nodedef));
		} else {
			ret = oldvalue;
		}
	} else {
		ret = brightness_sum / brightness_count;
	}

	*sunlight_seen_result = (sunlight_seen_count > 0);
	return ret;
}

void ClientMap::renderPostFx(CameraMode cam_mode)
{
	// Sadly ISceneManager has no "post effects" render pass, in that case we
	// could just register for that and handle it in renderMap().

	MapNode n = getNode(floatToInt(m_camera_position, BS));

	// - If the player is in a solid node, make everything black.
	// - If the player is in liquid, draw a semi-transparent overlay.
	// - Do not if player is in third person mode
	const ContentFeatures& features = m_nodedef->get(n);
	video::SColor post_effect_color = features.post_effect_color;
	if(features.solidness == 2 && !(g_settings->getBool("noclip") &&
			m_client->checkLocalPrivilege("noclip")) &&
			cam_mode == CAMERA_MODE_FIRST)
	{
		post_effect_color = video::SColor(255, 0, 0, 0);
	}
	if (post_effect_color.getAlpha() != 0)
	{
		// Draw a full-screen rectangle
		video::IVideoDriver* driver = SceneManager->getVideoDriver();
		v2u32 ss = driver->getScreenSize();
		core::rect<s32> rect(0,0, ss.X, ss.Y);
		driver->draw2DRectangle(post_effect_color, rect);
	}
}

void ClientMap::PrintInfo(std::ostream &out)
{
	out<<"ClientMap: ";
}

void ClientMap::renderMapShadows(video::IVideoDriver *driver,
		const video::SMaterial &material, s32 pass, int frame, int total_frames)
{
	bool is_transparent_pass = pass != scene::ESNRP_SOLID;
	std::string prefix;
	if (is_transparent_pass)
		prefix = "renderMap(SHADOW TRANS): ";
	else
		prefix = "renderMap(SHADOW SOLID): ";

	u32 drawcall_count = 0;
	u32 vertex_count = 0;

	MeshBufListList drawbufs;

	int count = 0;
	int low_bound = is_transparent_pass ? 0 : m_drawlist_shadow.size() / total_frames * frame;
	int high_bound = is_transparent_pass ? m_drawlist_shadow.size() : m_drawlist_shadow.size() / total_frames * (frame + 1);

	// transparent pass should be rendered in one go
	if (is_transparent_pass && frame != total_frames - 1) {
		return;
	}

	for (auto &i : m_drawlist_shadow) {
		// only process specific part of the list & break early
		++count;
		if (count <= low_bound)
			continue;
		if (count > high_bound)
			break;

		v3s16 block_pos = i.first;
		MapBlock *block = i.second;

		// If the mesh of the block happened to get deleted, ignore it
		if (!block->mesh)
			continue;

		/*
			Get the meshbuffers of the block
		*/
		{
			MapBlockMesh *mapBlockMesh = block->mesh;
			assert(mapBlockMesh);

			for (int layer = 0; layer < MAX_TILE_LAYERS; layer++) {
				scene::IMesh *mesh = mapBlockMesh->getMesh(layer);
				assert(mesh);

				u32 c = mesh->getMeshBufferCount();
				for (u32 i = 0; i < c; i++) {
					scene::IMeshBuffer *buf = mesh->getMeshBuffer(i);

					video::SMaterial &mat = buf->getMaterial();
					auto rnd = driver->getMaterialRenderer(mat.MaterialType);
					bool transparent = rnd && rnd->isTransparent();
					if (transparent == is_transparent_pass)
						drawbufs.add(buf, block_pos, layer);
				}
			}
		}
	}

	TimeTaker draw("Drawing shadow mesh buffers");

	core::matrix4 m; // Model matrix
	v3f offset = intToFloat(m_camera_offset, BS);

	// Render all layers in order
	for (auto &lists : drawbufs.lists) {
		for (MeshBufList &list : lists) {
			// Check and abort if the machine is swapping a lot
			if (draw.getTimerTime() > 1000) {
				infostream << "ClientMap::renderMapShadows(): Rendering "
						"took >1s, returning." << std::endl;
				break;
			}
			for (auto &pair : list.bufs) {
				scene::IMeshBuffer *buf = pair.second;

				// override some material properties
				video::SMaterial local_material = buf->getMaterial();
				local_material.MaterialType = material.MaterialType;
				local_material.BackfaceCulling = material.BackfaceCulling;
				local_material.FrontfaceCulling = material.FrontfaceCulling;
				local_material.BlendOperation = material.BlendOperation;
				local_material.Lighting = false;
				driver->setMaterial(local_material);

				v3f block_wpos = intToFloat(pair.first * MAP_BLOCKSIZE, BS);
				m.setTranslation(block_wpos - offset);

				driver->setTransform(video::ETS_WORLD, m);
				driver->drawMeshBuffer(buf);
				vertex_count += buf->getVertexCount();
			}

			drawcall_count += list.bufs.size();
		}
	}

	// restore the driver material state 
	video::SMaterial clean;
	clean.BlendOperation = video::EBO_ADD;
	driver->setMaterial(clean); // reset material to defaults
	driver->draw3DLine(v3f(), v3f(), video::SColor(0));
	
	g_profiler->avg(prefix + "draw meshes [ms]", draw.stop(true));
	g_profiler->avg(prefix + "vertices drawn [#]", vertex_count);
	g_profiler->avg(prefix + "drawcalls [#]", drawcall_count);
}

/*
	Custom update draw list for the pov of shadow light.
*/
void ClientMap::updateDrawListShadow(const v3f &shadow_light_pos, const v3f &shadow_light_dir, float shadow_range)
{
	ScopeProfiler sp(g_profiler, "CM::updateDrawListShadow()", SPT_AVG);

	const v3f camera_position = shadow_light_pos;
	const v3f camera_direction = shadow_light_dir;
	// I "fake" fov just to avoid creating a new function to handle orthographic
	// projection.
	const f32 camera_fov = m_camera_fov * 1.9f;

	v3s16 cam_pos_nodes = floatToInt(camera_position, BS);
	v3s16 p_blocks_min;
	v3s16 p_blocks_max;
	getBlocksInViewRange(cam_pos_nodes, &p_blocks_min, &p_blocks_max, shadow_range);

	std::vector<v2s16> blocks_in_range;

	for (auto &i : m_drawlist_shadow) {
		MapBlock *block = i.second;
		block->refDrop();
	}
	m_drawlist_shadow.clear();

	// We need to append the blocks from the camera POV because sometimes
	// they are not inside the light frustum and it creates glitches.
	// FIXME: This could be removed if we figure out why they are missing
	// from the light frustum.
	for (auto &i : m_drawlist) {
		i.second->refGrab();
		m_drawlist_shadow[i.first] = i.second;
	}

	// Number of blocks currently loaded by the client
	u32 blocks_loaded = 0;
	// Number of blocks with mesh in rendering range
	u32 blocks_in_range_with_mesh = 0;
	// Number of blocks occlusion culled
	u32 blocks_occlusion_culled = 0;

	for (auto &sector_it : m_sectors) {
		MapSector *sector = sector_it.second;
		if (!sector)
			continue;
		blocks_loaded += sector->size();

		MapBlockVect sectorblocks;
		sector->getBlocks(sectorblocks);

		/*
			Loop through blocks in sector
		*/
		for (MapBlock *block : sectorblocks) {
			if (!block->mesh) {
				// Ignore if mesh doesn't exist
				continue;
			}

			float range = shadow_range;

			float d = 0.0;
			if (!isBlockInSight(block->getPos(), camera_position,
					    camera_direction, camera_fov, range, &d))
				continue;

			blocks_in_range_with_mesh++;

			/*
				Occlusion culling
			*/
			if (isBlockOccluded(block, cam_pos_nodes)) {
				blocks_occlusion_culled++;
				continue;
			}

			// This block is in range. Reset usage timer.
			block->resetUsageTimer();

			// Add to set
			if (m_drawlist_shadow.find(block->getPos()) == m_drawlist_shadow.end()) {
				block->refGrab();
				m_drawlist_shadow[block->getPos()] = block;
			}
		}
	}

	g_profiler->avg("SHADOW MapBlock meshes in range [#]", blocks_in_range_with_mesh);
	g_profiler->avg("SHADOW MapBlocks occlusion culled [#]", blocks_occlusion_culled);
	g_profiler->avg("SHADOW MapBlocks drawn [#]", m_drawlist_shadow.size());
	g_profiler->avg("SHADOW MapBlocks loaded [#]", blocks_loaded);
}