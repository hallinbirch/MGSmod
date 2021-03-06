#include "mapdata.h"
#include "map.h"
#include "game.h"
#include "lightmap.h"

//CAT-mgs:
#define INBOUNDS(x, y) (x >= -CAT_VX && x <= CAT_VX && y >= -CAT_VY && y <= CAT_VY)
#define INBOUNDS_LARGE(x, y) (x >= -LIGHTMAP_RANGE_X && x <= LIGHTMAP_RANGE_X &&\
                               y >= -LIGHTMAP_RANGE_Y && y <= LIGHTMAP_RANGE_Y)

light_map::light_map()
 : lm()
 , sm()
{
 memset(lm, 0, sizeof(lm));
 memset(sm, 0, sizeof(sm));
}

void light_map::generate(game* g, int x, int y, float natural_light, float luminance)
{
 build_light_cache(g, x, y);
 memset(lm, 0, sizeof(lm));
 memset(sm, 0, sizeof(sm));

 int dir_x[] = { 1, 0 , -1,  0 };
 int dir_y[] = { 0, 1 ,  0, -1 };
 int dir_d[] = { 180, 270, 0, 90 };


//g->add_msg("natural_light %f", natural_light);

//CAT-mgs: this is just for indoors to initialize it at low ambient light
 if(natural_light > LIGHT_AMBIENT_LOW) // LIGHT_SOURCE_BRIGHT
 {
	for(int sx = x - CAT_VX; sx <= x + CAT_VX; ++sx)
	{
		for(int sy = y - CAT_VY; sy <= y + CAT_VY; ++sy)
		{
			if(!is_outside(sx - x + g->u.view_offset_x, sy - y + g->u.view_offset_y))
				lm[sx - x + CAT_VX][sy - y + CAT_VY] = LIGHT_AMBIENT_LOW;	
		}
	}
 }


 if(luminance > LIGHT_AMBIENT_LOW)
  apply_light_source(g->u.posx, g->u.posy, x, y, luminance);


 for(int sx = x - LIGHTMAP_RANGE_X; sx <= x + LIGHTMAP_RANGE_X; ++sx) {
  for(int sy = y - LIGHTMAP_RANGE_Y; sy <= y + LIGHTMAP_RANGE_Y; ++sy) {
   const ter_id terrain = g->m.ter(sx, sy);
   const std::vector<item> items = g->m.i_at(sx, sy);
   const field current_field = g->m.field_at(sx, sy);

   // When underground natural_light is 0, if this changes we need to revisit
   if (natural_light > LIGHT_AMBIENT_LOW) {

    int lx= sx - x  + g->u.view_offset_x;
    int ly= sy - y + g->u.view_offset_y;

    if(!is_outside(lx, ly)) {
     // Apply light sources for external/internal divide
     for(int i = 0; i < 4; ++i) {
      if(INBOUNDS_LARGE(lx + dir_x[i], ly + dir_y[i])
			&& is_outside(lx + dir_x[i], ly + dir_y[i]))
	{

//CAT-mgs: I did that above, didn't I?
//... no, yes, what's this for anyway?
		if(INBOUNDS(sx - x, sy - y) && is_outside(0, 0))
			lm[sx - x + CAT_VX][sy - y + CAT_VY]= natural_light;

		if(c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].transparency > LIGHT_TRANSPARENCY_SOLID)
			apply_light_arc(sx, sy, dir_d[i], x, y, natural_light);
      }
     }
    }
   }


//CAT-g:
   if(items.size() > 0)
   {
	   if(items[0].type->id == itm_flashlight_on
			|| items[items.size()-1].type->id == itm_flashlight_on)
		apply_light_source(sx, sy, x, y, 48); 

	   if(items[0].type->id == itm_torch_lit
			|| items[items.size()-1].type->id == itm_torch_lit)
		apply_light_source(sx, sy, x, y, 20);

	   if(items[0].type->id == itm_candle_lit
			|| items[items.size()-1].type->id == itm_candle_lit)
		apply_light_source(sx, sy, x, y, 9); 
   }


   if(terrain == t_lava)
    apply_light_source(sx, sy, x, y, 48);

   if(terrain == t_console)
    apply_light_source(sx, sy, x, y, 3);

   if(terrain == t_emergency_light)
    apply_light_source(sx, sy, x, y, 3);



   // TODO: [lightmap] Attach light brightness to fields
   switch(current_field.type) {
    case fd_fire:
     if(current_field.density > 5)
      apply_light_source(sx, sy, x, y, 48);
     else if (current_field.density > 2)
      apply_light_source(sx, sy, x, y, 20);
     else
      apply_light_source(sx, sy, x, y, 9);
     break;
    case fd_fire_vent:
      apply_light_source(sx, sy, x, y, 3);
    case fd_flame_burst:
      apply_light_source(sx, sy, x, y, 8);
     break;
    case fd_electricity:
     if (3 == current_field.density)
      apply_light_source(sx, sy, x, y, 9);
     else if (2 == current_field.density)
      apply_light_source(sx, sy, x, y, 1);
     else
      apply_light_source(sx, sy, x, y, LIGHT_SOURCE_LOCAL);  // kinda a hack as the square will still get marked
     break;
   }

   // Apply any vehicle light sources
   if (c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].veh &&
       c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].veh_light > LL_DARK) {
    if (c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].veh_light > LL_LIT) {
     int dir = c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].veh->face.dir();

//CAT-g: longer range headlights
//     float luminance = c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].veh_light;
	 float luminance = 4000 + c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].veh_light;    

	apply_light_arc(sx, sy, dir, x, y, luminance);
    }
   }

   if (c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].mon >= 0) {
    if (g->z[c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].mon].has_effect(ME_ONFIRE))
     apply_light_source(sx, sy, x, y, 3);

    // TODO: [lightmap] Attach natural light brightness to creatures
    // TODO: [lightmap] Allow creatures to have light attacks (ie: eyebot)
    // TODO: [lightmap] Allow creatures to have facing and arc lights
    switch(g->z[c[sx - x + LIGHTMAP_RANGE_X][sy - y + LIGHTMAP_RANGE_Y].mon].type->id) {
     case mon_zombie_electric:
      apply_light_source(sx, sy, x, y, 1);
      break;
     case mon_turret:
      apply_light_source(sx, sy, x, y, 2);
      break;
     case mon_flaming_eye:
      apply_light_source(sx, sy, x, y, LIGHT_SOURCE_BRIGHT);
      break;
     case mon_manhack:
      apply_light_source(sx, sy, x, y, LIGHT_SOURCE_LOCAL);
      break;
    }
   }
  }
 }

}



lit_level light_map::at(int dx, int dy)
{
//CAT-l: why does it not work upside down?

 if (!INBOUNDS(dx, dy))
  return LL_DARK; // Out of bounds

//CAT: check this
 if (sm[dx + CAT_VX][dy + CAT_VY] >= LIGHT_SOURCE_BRIGHT)
  return LL_BRIGHT;

 if (lm[dx + CAT_VX][dy + CAT_VY] >= LIGHT_AMBIENT_LIT)
  return LL_LIT;

 if (lm[dx + CAT_VX][dy + CAT_VY] >= LIGHT_AMBIENT_LOW)
  return LL_LOW;

 return LL_DARK;
}

float light_map::ambient_at(int dx, int dy)
{
 if (!INBOUNDS(dx, dy))
  return 0.0f;

 return lm[dx + CAT_VX][dy + CAT_VY];
}

bool light_map::is_outside(int dx, int dy)
{
 // We don't know and true seems a better default than false
 if (!INBOUNDS_LARGE(dx, dy))
  return true;

 return outside_cache[dx + LIGHTMAP_RANGE_X][dy + LIGHTMAP_RANGE_Y];
}

bool light_map::sees(int fx, int fy, int tx, int ty, int max_range)
{
 if(!INBOUNDS_LARGE(fx, fy) || !INBOUNDS_LARGE(tx, ty))
  return false;

 if (max_range >= 0 && (abs(tx - fx) > max_range || abs(ty - fy) > max_range))
  return false; // Out of range!

 int ax = abs(tx - fx) << 1;
 int ay = abs(ty - fy) << 1;
 int dx = (fx < tx) ? 1 : -1;
 int dy = (fy < ty) ? 1 : -1;
 int x = fx;
 int y = fy;

 // TODO: [lightmap] Pull out the common code here rather than duplication
 if (ax > ay) {
  int t = ay - (ax >> 1);
  do {
   if(t >= 0 && ((y + dy != ty) || (x + dx == tx))) {
    y += dy;
    t -= ax;
   }

   x += dx;
   t += ay;

   if(c[x + LIGHTMAP_RANGE_X][y + LIGHTMAP_RANGE_Y].transparency == LIGHT_TRANSPARENCY_SOLID)
    break;

  } while(!(x == tx && y == ty));
 } else {
  int t = ax - (ay >> 1);
  do {
   if(t >= 0 && ((x + dx != tx) || (y + dy == ty))) {
    x += dx;
    t -= ay;
   }

   y += dy;
   t += ax;

   if(c[x + LIGHTMAP_RANGE_X][y + LIGHTMAP_RANGE_Y].transparency == LIGHT_TRANSPARENCY_SOLID)
    break;

  } while(!(x == tx && y == ty));
 }

 return (x == tx && y == ty);
}

void light_map::apply_light_source(int x, int y, int cx, int cy, float luminance)
{
 bool lit[LIGHTMAP_X][LIGHTMAP_Y];
 memset(lit, 0, sizeof(lit));

 if (INBOUNDS(x - cx, y - cy)) {
  lit[x - cx + CAT_VX][y - cy + CAT_VY] = true;
  lm[x - cx + CAT_VX][y - cy + CAT_VY] += std::max(luminance, static_cast<float>(LL_LOW));
  sm[x - cx + CAT_VX][y - cy + CAT_VY] += luminance;
 }

 if (luminance > LIGHT_SOURCE_LOCAL) {
  int range = LIGHT_RANGE(luminance);
  int sx = x - cx - range; int ex = x - cx + range;
  int sy = y - cy - range; int ey = y - cy + range;

  for(int off = sx; off <= ex; ++off) {
   apply_light_ray(lit, x, y, cx + off, cy + sy, cx, cy, luminance);
   apply_light_ray(lit, x, y, cx + off, cy + ey, cx, cy, luminance);
  }

  // Skip corners with + 1 and < as they were done
  for(int off = sy + 1; off < ey; ++off) {
   apply_light_ray(lit, x, y, cx + sx, cy + off, cx, cy, luminance);
   apply_light_ray(lit, x, y, cx + ex, cy + off, cx, cy, luminance);
  }
 }
}

void light_map::apply_light_arc(int x, int y, int angle, int cx, int cy, float luminance)
{
 if (luminance <= LIGHT_SOURCE_LOCAL)
  return;

 bool lit[LIGHTMAP_X][LIGHTMAP_Y];
 memset(lit, 0, sizeof(lit));

 int range = LIGHT_RANGE(luminance);
 apply_light_source(x, y, cx, cy, LIGHT_SOURCE_LOCAL);

 // Normalise (should work with negative values too)
 angle = angle % 360;

 // East side
 if (angle < 90 || angle > 270) {
  int sy = y - cy - ((angle <  90) ? range * (( 45 - angle) / 45.0f) : range);
  int ey = y - cy + ((angle > 270) ? range * ((angle - 315) / 45.0f) : range);

  int ox = x - cx + range;
  for(int oy = sy; oy <= ey; ++oy)
   apply_light_ray(lit, x, y, cx + ox, cy + oy, cx, cy, luminance);
 }

 // South side
 if (angle < 180) {
  int sx = x - cx - ((angle < 90) ? range * (( angle - 45) / 45.0f) : range);
  int ex = x - cx + ((angle > 90) ? range * ((135 - angle) / 45.0f) : range);

  int oy = y - cy + range;
  for(int ox = sx; ox <= ex; ++ox)
   apply_light_ray(lit, x, y, cx + ox, cy + oy, cx, cy, luminance);
 }

 // West side
 if (angle > 90 && angle < 270) {
  int sy = y - cy - ((angle < 180) ? range * ((angle - 135) / 45.0f) : range);
  int ey = y - cy + ((angle > 180) ? range * ((225 - angle) / 45.0f) : range);

  int ox = x - cx - range;
  for(int oy = sy; oy <= ey; ++oy)
   apply_light_ray(lit, x, y, cx + ox, cy + oy, cx, cy, luminance);
 }

 // North side
 if (angle > 180) {
  int sx = x - cx - ((angle > 270) ? range * ((315 - angle) / 45.0f) : range);
  int ex = x - cx + ((angle < 270) ? range * ((angle - 225) / 45.0f) : range);

  int oy = y - cy - range;
  for(int ox = sx; ox <= ex; ++ox)
   apply_light_ray(lit, x, y, cx + ox, cy + oy, cx, cy, luminance);
 }
}


//CAT-l: *** c/p whole lot ***
void light_map::apply_light_ray(bool lit[LIGHTMAP_X][LIGHTMAP_Y], int sx, int sy,
                                int ex, int ey, int cx, int cy, float luminance)
{
 int ax = abs(ex - sx) << 1;
 int ay = abs(ey - sy) << 1;
 int dx = (sx < ex) ? 1 : -1;
 int dy = (sy < ey) ? 1 : -1;
 int x = sx;
 int y = sy;

 float transparency = LIGHT_TRANSPARENCY_CLEAR;

 // TODO: [lightmap] Pull out the common code here rather than duplication
 if (ax > ay) {

  int t = ay - (ax >> 1);
  do {
   if(t >= 0) {
    y += dy;
    t -= ax;
   }

   x += dx;
   t += ay;

//CAT:
   float light = luminance / ((sx - x) * (sx - x));
   float cat_rng= sqrt( ((sx - x) * (sx - x)) + ((sy - y) * (sy - y)) );

   if(INBOUNDS(x - cx, y - cy) && !lit[x - cx + CAT_VX][y - cy + CAT_VY] && cat_rng <= light*2+2)
   {
     //Multiple rays will pass through the same squares so we need to record that
     lit[x - cx + CAT_VX][y - cy + CAT_VY] = true;
     lm[x - cx + CAT_VX][y - cy + CAT_VY] += light * transparency;
   }

   if(INBOUNDS_LARGE(x - cx, y - cy))
    transparency *= c[x - cx + LIGHTMAP_RANGE_X][y - cy + LIGHTMAP_RANGE_Y].transparency;

   if (transparency <= LIGHT_TRANSPARENCY_SOLID)
    break;

  } while(!(x == ex && y == ey));

 }else {

  int t = ax - (ay >> 1);
  do {
   if(t >= 0) {
    x += dx;
    t -= ay;
   }

   y += dy;
   t += ax;

//CAT: 
   float light = luminance / ((sy - y) * (sy - y));
   float cat_rng= sqrt( ((sx - x) * (sx - x)) + ((sy - y) * (sy - y)) );

   if(INBOUNDS(x - cx, y - cy) && !lit[x - cx + CAT_VX][y - cy + CAT_VY] && cat_rng <= light*2+2)
   {
     //Multiple rays will pass through the same squares so we need to record that
     lit[x - cx + CAT_VX][y - cy + CAT_VY] = true;
     lm[x - cx + CAT_VX][y - cy + CAT_VY] += light;
   }


   if (INBOUNDS_LARGE(x - cx, y - cy))
    transparency *= c[x - cx + LIGHTMAP_RANGE_X][y - cy + LIGHTMAP_RANGE_Y].transparency;

   if (transparency <= LIGHT_TRANSPARENCY_SOLID)
    break;

  }while(!(x == ex && y == ey));

 }
}


void light_map::build_outside_cache(map *m, const int x, const int y, const int sx, const int sy)
{
 const ter_id terrain = m->ter(sx, sy);

 if( terrain == t_floor || terrain == t_rock_floor || terrain == t_floor_wax 
	|| terrain == t_fema_groundsheet || terrain == t_dirtfloor
	|| terrain == t_elevator || terrain == t_table || terrain == t_chair 
	|| terrain == t_bench || terrain == t_desk || terrain == t_counter 
	|| terrain == t_indoor_hole ) //|| terrain == t_rock
 {

	for( int dx = -1; dx <= 1; dx++ )
	{
	   for( int dy = -1; dy <= 1; dy++ )
	    outside_cache[x + dx][y + dy] = false;
	}
 }

/*
 else
 if(terrain == t_bed || terrain == t_groundsheet || terrain == t_makeshift_bed)
  outside_cache[x][y] = false;
*/
 
}

// We only do this once now so we don't make 100k calls to is_outside for each
// generation. As well as close to that for the veh_at function.
void light_map::build_light_cache(game* g, int cx, int cy)
{
 // Clear cache
 memset(outside_cache, true, sizeof(outside_cache));

 for(int x = 0; x < LIGHTMAP_CACHE_X; x++) {
  for(int y = 0; y < LIGHTMAP_CACHE_Y; y++) {
   int sx = x + g->u.posx - LIGHTMAP_RANGE_X;
   int sy = y + g->u.posy - LIGHTMAP_RANGE_Y;

   build_outside_cache(&g->m, x, y, sx, sy);
   c[x][y].transparency = LIGHT_TRANSPARENCY_CLEAR;
   c[x][y].veh = NULL;
   c[x][y].veh_part = 0;
   c[x][y].veh_light = 0;
   c[x][y].mon = -1;
  }
 }

 // Check for critters and cache
 for (int i = 0; i < g->z.size(); ++i)
  if (INBOUNDS(g->z[i].posx - cx, g->z[i].posy - cy))
   c[g->z[i].posx - cx + LIGHTMAP_RANGE_X][g->z[i].posy - cy + LIGHTMAP_RANGE_Y].mon = i;

 // Check for vehicles and cache
 VehicleList vehs = g->m.get_vehicles(cx - LIGHTMAP_RANGE_X, cy - LIGHTMAP_RANGE_Y, cx + LIGHTMAP_RANGE_X, cy + LIGHTMAP_RANGE_Y);
 for(int v = 0; v < vehs.size(); ++v) {
  for(int p = 0; p < vehs[v].v->parts.size(); ++p) {
   int px = vehs[v].x + vehs[v].v->parts[p].precalc_dx[0] - cx;
   int py = vehs[v].y + vehs[v].v->parts[p].precalc_dy[0] - cy;

//CAT-l: fix bug with dissapearing headlight
//    if(INBOUNDS(px, py)) 
   if(INBOUNDS_LARGE(px, py))
   {
    px += LIGHTMAP_RANGE_X;
    py += LIGHTMAP_RANGE_Y;

    if (vehs[v].v->is_inside(p))
     outside_cache[px][py] = true;
    // External part appears to always be the first?
    if (!c[px][py].veh) {
     c[px][py].veh = vehs[v].v;
     c[px][py].veh_part = p;
    }

    if (vehs[v].v->lights_on &&
        vehs[v].v->part_flag(p, vpf_light) &&
        vehs[v].v->parts[p].hp > 0)
     c[px][py].veh_light = vehs[v].v->part_info(p).power;
   }
  }
 }

 for(int x = 0; x < LIGHTMAP_CACHE_X; x++) {
  for(int y = 0; y < LIGHTMAP_CACHE_Y; y++) {
   int sx = x + cx - LIGHTMAP_RANGE_X;
   int sy = y + cy - LIGHTMAP_RANGE_Y;

   if (c[x][y].veh) {
    if (c[x][y].veh->part_flag(c[x][y].veh_part, vpf_opaque) &&
        c[x][y].veh->parts[c[x][y].veh_part].hp > 0) {
     int dpart = c[x][y].veh->part_with_feature(c[x][y].veh_part, vpf_openable);
     if (dpart < 0 || !c[x][y].veh->parts[dpart].open) {
      c[x][y].transparency = LIGHT_TRANSPARENCY_SOLID;
      continue;
     }
    }
   } else if (!(terlist[g->m.ter(sx, sy)].flags & mfb(transparent))) {
    c[x][y].transparency = LIGHT_TRANSPARENCY_SOLID;
    continue;
   }

   field& f = g->m.field_at(sx, sy);
   if(f.type > 0) {
    if(!fieldlist[f.type].transparent[f.density - 1]) {
     // Fields are either transparent or not, however we want some to be translucent
     switch(f.type) {
      case fd_smoke:
      case fd_toxic_gas:
      case fd_tear_gas:
       if(f.density == 3)
        c[x][y].transparency = LIGHT_TRANSPARENCY_SOLID;
       if(f.density == 2)
        c[x][y].transparency *= 0.5;
       break;
      case fd_nuke_gas:
       c[x][y].transparency *= 0.5;
       break;
      default:
       c[x][y].transparency = LIGHT_TRANSPARENCY_SOLID;
       break;
     }
    }

    // TODO: [lightmap] Have glass reduce light as well
   }
  }
 }
}
