#include "draw.hpp"

#include <algorithm>

#define IM_PI 3.14159265358979323846f

void
overlay::draw::text (const ImVec2 &pos, const std::string &text, bool centered,
		     bool outline, color col)
{
  constexpr float thickness = 1.f;
  const ImVec2 offsets[]
    = {{-thickness, -thickness}, {0, -thickness},	{thickness, -thickness},
       {-thickness, 0},		 {thickness, 0},	{-thickness, thickness},
       {0, thickness},		 {thickness, thickness}};

  if (!centered)
    {
      if (outline)
	{
	  for (const auto &offset : offsets)
	    ImGui::GetWindowDrawList ()->AddText ({pos.x + offset.x,
						   pos.y + offset.y},
						  color ("#000000ff"),
						  text.c_str ());
	}

      ImGui::GetWindowDrawList ()->AddText (pos, col, text.c_str ());
    }
  else
    {
      auto tex_size
	= ImGui::GetFont ()->CalcTextSizeA (ImGui::GetFont ()->FontSize,
					    FLT_MAX, 0, text.c_str ());
      auto new_pos = ImVec2 (pos.x - tex_size.x * 0.5f,
			     pos.y - ImGui::GetFont ()->FontSize * 0.5f);

      if (outline)
	{
	  for (const auto &offset : offsets)
	    ImGui::GetWindowDrawList ()->AddText ({new_pos.x + offset.x,
						   new_pos.y + offset.y},
						  color ("#000000ff"),
						  text.c_str ());
	}

      ImGui::GetWindowDrawList ()->AddText (new_pos, col, text.c_str ());
    }
}

void
overlay::draw::rectangle (const ImVec2 &start, const ImVec2 &end, bool filled,
			  color col)
{
  if (filled)
    {
      ImGui::GetWindowDrawList ()->AddRectFilled (start, end, col, 1.5);
    }
  else
    {
      ImGui::GetWindowDrawList ()->AddRect (start, end, col, 1.5);
    }
}

void
overlay::draw::circle (const ImVec2 &center, float radius, bool filled,
		       float thickness, color col)
{
  if (filled)
    {
      ImGui::GetWindowDrawList ()->AddCircleFilled (center, radius, col, 100);
    }
  else
    {
      ImGui::GetWindowDrawList ()->AddCircle (center, radius, col, 100,
					      thickness);
    }
}

void
overlay::draw::line (const ImVec2 &start, const ImVec2 &end, float thickness,
		     color col)
{
  auto *dl = ImGui::GetWindowDrawList ();
  dl->AddLine (start, end, col, thickness);
  dl->AddCircleFilled (start, thickness * 0.5f, col, 12);
  dl->AddCircleFilled (end, thickness * 0.5f, col, 12);
}

ImVec2
catmull_rom (const ImVec2 &p0, const ImVec2 &p1, const ImVec2 &p2,
	     const ImVec2 &p3, float t)
{
  float t2 = t * t;
  float t3 = t2 * t;

  return {0.5f
	    * ((2.0f * p1.x) + (-p0.x + p2.x) * t
	       + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2
	       + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3),
	  0.5f
	    * ((2.0f * p1.y) + (-p0.y + p2.y) * t
	       + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2
	       + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3)};
}

std::vector<ImVec2>
smooth_path (const std::vector<ImVec2> &points, float smooth)
{
  std::vector<ImVec2> out;

  if (points.size () < 4)
    return points;

  int steps = static_cast<int> (smooth);

  for (size_t i = 1; i + 2 < points.size (); ++i)
    {
      const ImVec2 &p0 = points[i - 1];
      const ImVec2 &p1 = points[i];
      const ImVec2 &p2 = points[i + 1];
      const ImVec2 &p3 = points[i + 2];

      for (int j = 0; j <= steps; ++j)
	{
	  float t = static_cast<float> (j) / static_cast<float> (steps);
	  out.push_back (catmull_rom (p0, p1, p2, p3, t));
	}
    }

  return out;
}

void
overlay::draw::path (const std::vector<ImVec2> &points, float smooth,
		     float thickness, color col)
{
  if (points.size () < 2)
    return;

  auto *draw_list = ImGui::GetWindowDrawList ();
  if (!draw_list)
    return;

  if (points.size () < 3 || smooth < 1.0f)
    {
      draw_list->AddPolyline (points.data (), static_cast<int> (points.size ()),
			      col, ImDrawFlags_None, thickness);
      draw_list->AddCircleFilled (points.front (), thickness * 0.5f, col, 12);
      draw_list->AddCircleFilled (points.back (), thickness * 0.5f, col, 12);
      return;
    }

  // --- CCMA parameters from smooth factor ---
  // smooth controls overall strength: w_ma = smooth, w_cc = max(1, smooth/2)
  int w_ma = static_cast<int> (smooth);
  int w_cc = std::max (1, w_ma / 2);
  int w_ccma = w_ma + w_cc + 1;

  // --- Generate Hanning kernel weights ---
  // Returns normalized hanning weights for a given window size
  auto make_hanning = [] (int window_size) -> std::vector<float> {
    if (window_size <= 0)
      return {1.0f};
    if (window_size == 1)
      return {1.0f};
    int n = window_size + 2; // hanning adds 2 then strips edges
    std::vector<float> k (window_size);
    float sum = 0.0f;
    for (int i = 0; i < window_size; i++)
      {
	k[i] = 0.5f * (1.0f - cosf (2.0f * IM_PI * (i + 1) / (n - 1)));
	sum += k[i];
      }
    for (auto &v : k)
      v /= sum;
    return k;
  };

  // Pre-compute weight lists for MA and CC at each sub-width
  // weights_ma[i] is the hanning kernel of size (2*i+1)
  std::vector<std::vector<float>> weights_ma (w_ma + 1);
  for (int i = 0; i <= w_ma; i++)
    weights_ma[i] = make_hanning (2 * i + 1);

  std::vector<std::vector<float>> weights_cc (w_cc + 1);
  for (int i = 0; i <= w_cc; i++)
    weights_cc[i] = make_hanning (2 * i + 1);

  // --- Helper: 3D vector math (we work in 3D internally, z=0 for 2D) ---
  struct v3
  {
    float x, y, z;
    v3 () : x (0), y (0), z (0) {}
    v3 (float x, float y, float z) : x (x), y (y), z (z) {}
    v3 operator+ (const v3 &o) const { return {x + o.x, y + o.y, z + o.z}; }
    v3 operator- (const v3 &o) const { return {x - o.x, y - o.y, z - o.z}; }
    v3 operator* (float s) const { return {x * s, y * s, z * s}; }
    v3 &operator+= (const v3 &o)
    {
      x += o.x;
      y += o.y;
      z += o.z;
      return *this;
    }
    float length () const { return sqrtf (x * x + y * y + z * z); }
    v3 normalized () const
    {
      float l = length ();
      return l == 0.0f ? v3{} : v3{x / l, y / l, z / l};
    }
  };

  auto cross = [] (const v3 &a, const v3 &b) -> v3 {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
	    a.x * b.y - a.y * b.x};
  };

  // --- Build padded point array (padding mode) ---
  int n_orig = static_cast<int> (points.size ());
  int n_pad = w_ccma;
  int n_total = n_orig + 2 * n_pad;

  std::vector<v3> pts (n_total);
  for (int i = 0; i < n_pad; i++)
    pts[i] = {points[0].x, points[0].y, 0.0f};
  for (int i = 0; i < n_orig; i++)
    pts[i + n_pad] = {points[i].x, points[i].y, 0.0f};
  for (int i = 0; i < n_pad; i++)
    pts[n_pad + n_orig + i]
      = {points[n_orig - 1].x, points[n_orig - 1].y, 0.0f};

  // Add tiny noise to avoid degenerate straight-line cases
  for (auto &p : pts)
    {
      p.x += ((float) rand () / RAND_MAX) * 1e-6f;
      p.y += ((float) rand () / RAND_MAX) * 1e-6f;
    }

  // --- Convolution (moving average on points) ---
  auto convolve_points = [] (const std::vector<v3> &p,
			     const std::vector<float> &w) -> std::vector<v3> {
    int n = static_cast<int> (p.size ());
    int k = static_cast<int> (w.size ());
    int out_n = n - k + 1;
    if (out_n <= 0)
      return {};
    std::vector<v3> out (out_n);
    for (int i = 0; i < out_n; i++)
      {
	v3 sum;
	for (int j = 0; j < k; j++)
	  sum += p[i + j] * w[j];
	out[i] = sum;
      }
    return out;
  };

  // --- Step 1: Moving average ---
  std::vector<v3> pts_ma = convolve_points (pts, weights_ma[w_ma]);

  int n_ma = static_cast<int> (pts_ma.size ());
  if (n_ma < 3)
    {
      // Fallback: just draw original
      draw_list->AddPolyline (points.data (), static_cast<int> (points.size ()),
			      col, ImDrawFlags_None, thickness);
      return;
    }

  // --- Step 2: Curvature vectors ---
  std::vector<v3> curv_vecs (n_ma);
  std::vector<float> curvatures (n_ma, 0.0f);

  for (int i = 1; i < n_ma - 1; i++)
    {
      v3 v1 = pts_ma[i] - pts_ma[i - 1];
      v3 v2 = pts_ma[i + 1] - pts_ma[i];
      v3 cr = cross (v1, v2);
      float cr_norm = cr.length ();

      if (cr_norm != 0.0f)
	{
	  float d01 = v1.length ();
	  float d12 = v2.length ();
	  float d02 = (pts_ma[i + 1] - pts_ma[i - 1]).length ();
	  float radius = (d01 * d12 * d02) / (2.0f * cr_norm);
	  curvatures[i] = 1.0f / radius;
	  curv_vecs[i] = cr.normalized () * curvatures[i];
	}
    }

  // --- Step 3: Alphas (angles between consecutive points on circumcircle) ---
  std::vector<float> alphas (n_ma, 0.0f);
  for (int i = 1; i < n_ma - 1; i++)
    {
      if (curvatures[i] != 0.0f)
	{
	  float radius = 1.0f / curvatures[i];
	  float dist = (pts_ma[i + 1] - pts_ma[i - 1]).length ();
	  float arg = (dist * 0.5f) / radius;
	  alphas[i] = (arg >= 1.0f) ? (IM_PI * 0.5f) : asinf (arg);
	}
    }

  // --- Step 4: Normalized MA radii ---
  std::vector<float> radii_ma (n_ma, 0.0f);
  const auto &w_ma_weights = weights_ma[w_ma];

  for (int i = 1; i < n_ma - 1; i++)
    {
      float r = 1.0f * w_ma_weights[w_ma]; // central weight
      for (int k = 1; k <= w_ma; k++)
	r += 2.0f * cosf (alphas[i] * k) * w_ma_weights[w_ma + k];
      radii_ma[i] = std::max (0.35f, r);
    }

  // --- Step 5: Curvature correction ---
  int n_ccma = n_total - 2 * w_ccma;
  if (n_ccma <= 0)
    {
      draw_list->AddPolyline (points.data (), static_cast<int> (points.size ()),
			      col, ImDrawFlags_None, thickness);
      return;
    }

  std::vector<ImVec2> smoothed (n_ccma);
  const auto &w_cc_weights = weights_cc[w_cc];

  for (int i = 0; i < n_ccma; i++)
    {
      // Tangent vector at this point in MA space
      int ma_idx = i + w_cc + 1;
      v3 tangent = (pts_ma[ma_idx + 1] - pts_ma[ma_idx - 1]).normalized ();

      // Weighted shift from curvature correction
      v3 shift;
      for (int j = 0; j < 2 * w_cc + 1; j++)
	{
	  int ci = i + j + 1; // index into curvature arrays
	  if (curvatures[ci] == 0.0f)
	    continue;

	  v3 u = curv_vecs[ci].normalized ();
	  float weight = w_cc_weights[j];
	  float shift_mag
	    = (1.0f / curvatures[ci]) * (1.0f / radii_ma[ci] - 1.0f);
	  shift += u * (weight * shift_mag);
	}

      // Reconstruct: point + cross(tangent, shift)
      v3 corrected = pts_ma[ma_idx] + cross (tangent, shift);
      smoothed[i] = {corrected.x, corrected.y};
    }

  draw_list->AddPolyline (smoothed.data (), static_cast<int> (smoothed.size ()),
			  col, ImDrawFlags_None, thickness);
  draw_list->AddCircleFilled (smoothed.front (), thickness * 0.5f, col, 12);
  draw_list->AddCircleFilled (smoothed.back (), thickness * 0.5f, col, 12);
}