#include "line_processor.h"

#include <math.h>
#include <float.h>
#include <iostream>
#include <numeric>

#include "timer.h"

INITIALIZE_TIMER;

void FilterShortLines(std::vector<Eigen::Vector4f>& lines, float length_thr){
  Eigen::Array4Xf line_array = Eigen::Map<Eigen::Array4Xf, Eigen::Unaligned>(lines[0].data(), 4, lines.size());
  Eigen::ArrayXf length_square = (line_array.row(2) - line_array.row(0)).square() + (line_array.row(3) - line_array.row(1)).square();
  float thr_square = length_thr * length_thr;

  size_t long_line_num = 0;
  for(size_t i = 0; i < lines.size(); i++){
    if(length_square(i) > thr_square){
      lines[long_line_num] = lines[i];
      long_line_num++;
    }
  }
  lines.resize(long_line_num);
}

void FilterShortLines(std::vector<Eigen::Vector4d>& lines, float length_thr){
  Eigen::Array4Xd line_array = Eigen::Map<Eigen::Array4Xd, Eigen::Unaligned>(lines[0].data(), 4, lines.size());
  Eigen::ArrayXd length_square = (line_array.row(2) - line_array.row(0)).square() + (line_array.row(3) - line_array.row(1)).square();
  float thr_square = length_thr * length_thr;

  size_t long_line_num = 0;
  for(size_t i = 0; i < lines.size(); i++){
    if(length_square(i) > thr_square){
      lines[long_line_num] = lines[i];
      long_line_num++;
    }
  }
  lines.resize(long_line_num);
}

float AngleDiff(float& angle1, float& angle2){
  float d_angle_case1 = std::abs(angle2 - angle1);
  float d_angle_case2 = M_PI + std::min(angle1, angle2) - std::max(angle1, angle2);
  return std::min(d_angle_case1, d_angle_case2);
}

Eigen::Vector4f MergeTwoLines(const Eigen::Vector4f& line1, const Eigen::Vector4f& line2){
  double xg = 0.0, yg = 0.0;
  double delta1x = 0.0, delta1y = 0.0, delta2x = 0.0, delta2y = 0.0;
  float ax = 0, bx = 0, cx = 0, dx = 0;
  float ay = 0, by = 0, cy = 0, dy = 0;
  double li = 0.0, lj = 0.0;
  double thi = 0.0, thj = 0.0, thr = 0.0;
  double axg = 0.0, bxg = 0.0, cxg = 0.0, dxg = 0.0, delta1xg = 0.0, delta2xg = 0.0;

  ax = line1(0);
  ay = line1(1);
  bx = line1(2);
  by = line1(3);

  cx = line2(0);
  cy = line2(1);
  dx = line2(2);
  dy = line2(3);

  float dlix = (bx - ax);
  float dliy = (by - ay);
  float dljx = (dx - cx);
  float dljy = (dy - cy);

  li = sqrt((double) (dlix * dlix) + (double) (dliy * dliy));
  lj = sqrt((double) (dljx * dljx) + (double) (dljy * dljy));

  xg = (li * (double) (ax + bx) + lj * (double) (cx + dx))
      / (double) (2.0 * (li + lj));
  yg = (li * (double) (ay + by) + lj * (double) (cy + dy))
      / (double) (2.0 * (li + lj));

  if(dlix == 0.0f) thi = CV_PI / 2.0;
  else thi = atan(dliy / dlix);

  if(dljx == 0.0f) thj = CV_PI / 2.0;
  else thj = atan(dljy / dljx);

  if (fabs(thi - thj) <= CV_PI / 2.0){
      thr = (li * thi + lj * thj) / (li + lj);
  }
  else{
      double tmp = thj - CV_PI * (thj / fabs(thj));
      thr = li * thi + lj * tmp;
      thr /= (li + lj);
  }

  axg = ((double) ay - yg) * sin(thr) + ((double) ax - xg) * cos(thr);
  bxg = ((double) by - yg) * sin(thr) + ((double) bx - xg) * cos(thr);
  cxg = ((double) cy - yg) * sin(thr) + ((double) cx - xg) * cos(thr);
  dxg = ((double) dy - yg) * sin(thr) + ((double) dx - xg) * cos(thr);

  delta1xg = std::min(axg, std::min(bxg, std::min(cxg,dxg)));
  delta2xg = std::max(axg, std::max(bxg, std::max(cxg,dxg)));

  delta1x = delta1xg * std::cos(thr) + xg;
  delta1y = delta1xg * std::sin(thr) + yg;
  delta2x = delta2xg * std::cos(thr) + xg;
  delta2y = delta2xg * std::sin(thr) + yg;

  Eigen::Vector4f new_line;
  new_line << (float)delta1x, (float)delta1y, (float)delta2x, (float)delta2y;
  return new_line;
}

void AssignPointsToLines(std::vector<Eigen::Vector4d>& lines, Eigen::Matrix2Xd& points, std::vector<std::set<int>>& relation){
  Eigen::Array2Xd point_array = points.array();
  Eigen::Array4Xd line_array = Eigen::Map<Eigen::Array4Xd, Eigen::Unaligned>(lines[0].data(), 4, lines.size());

  Eigen::ArrayXd x = point_array.row(0);
  Eigen::ArrayXd y = point_array.row(1); 

  Eigen::ArrayXd x1 = line_array.row(0);
  Eigen::ArrayXd y1 = line_array.row(1);
  Eigen::ArrayXd x2 = line_array.row(2);
  Eigen::ArrayXd y2 = line_array.row(3);

  Eigen::ArrayXd A = y2 - y1;
  Eigen::ArrayXd B = x1 - x2;
  Eigen::ArrayXd C = x2 * y1 - x1 * y2;
  Eigen::ArrayXd D = (A.square() + B.square()).sqrt();

  // Eigen::ArrayXXd line_matrix(A.rows(), 3);
  // line_matrix << A, B, C;
  // Eigen::ArrayXXd point_marix(x.rows(), 3);
  // point_marix << x, y, Eigen::ArrayXd::Ones(x.rows());

  // // Eigen::MatrixXd distances = A.matrix().transpose() * x.matrix() + B.matrix().transpose() * y.matrix() + C.matrix().transpose();
  // Eigen::MatrixXd distances = line_matrix.matrix() * point_marix.matrix().transpose();
  // auto good_distances = (distances.array() <= 3.0);

  relation.clear();
  relation.reserve(lines.size());
  for(int i = 0, line_num = lines.size(); i < line_num; i++){
    std::set<int> points_on_line;
    for(int j = 0, point_num = points.cols(); j < point_num; j++){
      // if(!good_distances(i, j)) continue;
      // filter by x, y
      double lx1 = x1(i);
      double ly1 = y1(i);
      double lx2 = x2(i);
      double ly2 = y2(i);
      double px = x(j);
      double py = y(j);
      
      double min_lx = lx1;
      double max_lx = lx2;
      double min_ly = ly1;
      double max_ly = ly2;
      if(lx1 > lx2) std::swap(min_lx, max_lx);
      if(ly1 > ly2) std::swap(min_ly, max_ly);
      if(px < min_lx - 3 || px > max_lx + 3 || py < min_ly - 3 || py > max_ly + 3) continue;

      // check distance
      float pl_distance = std::abs((A(i) * px + B(i) * py + C(i))) / D(i);
      if(pl_distance > 3) continue;

      double side1 = std::pow((lx1 - px), 2) + std::pow((ly1 - py), 2);
      double side2 = std::pow((lx2 - px), 2) + std::pow((ly2 - py), 2);
      double line_side = std::pow(D(i), 2);
      if(side1 <= 9 || side2 <= 9 || ((side1 < line_side + side2) && (side2 < line_side + side1))){
        points_on_line.insert(j);
      }
    }
    relation.push_back(points_on_line);
  }
}

LineDetector::LineDetector(const LineDetectorConfig &line_detector_config): _line_detector_config(line_detector_config){
  fld = cv::ximgproc::createFastLineDetector(line_detector_config.length_threshold, line_detector_config.distance_threshold, 
      line_detector_config.canny_th1, line_detector_config.canny_th2, line_detector_config.canny_aperture_size, line_detector_config.do_merge);
      // line_detector_config.canny_th1, line_detector_config.canny_th2, line_detector_config.canny_aperture_size, false);
}

void LineDetector::LineExtractor(cv::Mat& image, std::vector<Eigen::Vector4d>& lines){
  START_TIMER;
  std::vector<Eigen::Vector4f> source_lines, dst_lines;
  std::vector<cv::Vec4f> cv_lines;
  fld->detect(image, cv_lines);
  for(auto& cv_line : cv_lines){
    source_lines.emplace_back(cv_line[0], cv_line[1], cv_line[2], cv_line[3]);
  }
  STOP_TIMER("Line detect");
  START_TIMER;

  if(_line_detector_config.do_merge){
    std::vector<Eigen::Vector4f> tmp_lines;
    FilterShortLines(lines, 5);
    MergeLines(source_lines, tmp_lines, 0.05, 5, 15);
    FilterShortLines(tmp_lines, 20);
    std::cout << "second merge" << std::endl;
    MergeLines(tmp_lines, dst_lines, 0.05, 5, 30);
    FilterShortLines(dst_lines, 50);

    // MergeLines(source_lines, tmp_lines, 0.05, 5, 15, 0.2);
    // FilterShortLines(tmp_lines, 20);
    // MergeLines(tmp_lines, dst_lines, 0.05, 5, 40, 0.2);
    // FilterShortLines(dst_lines, 60);

    for(auto& line : dst_lines){
      lines.push_back(line.cast<double>());
    }
  }else{
    for(auto& line : source_lines){
      lines.push_back(line.cast<double>());
    }
  }
  STOP_TIMER("Merge");
}

void LineDetector::MergeLines(std::vector<Eigen::Vector4f>& source_lines, std::vector<Eigen::Vector4f>& dst_lines,
    float angle_threshold, float distance_threshold, float endpoint_threshold){

  size_t source_line_num = source_lines.size();
  Eigen::Array4Xf line_array = Eigen::Map<Eigen::Array4Xf, Eigen::Unaligned>(source_lines[0].data(), 4, source_lines.size());
  Eigen::ArrayXf x1 = line_array.row(0);
  Eigen::ArrayXf y1 = line_array.row(1);
  Eigen::ArrayXf x2 = line_array.row(2);
  Eigen::ArrayXf y2 = line_array.row(3);

  Eigen::ArrayXf dx = x2 - x1;
  Eigen::ArrayXf dy = y2 - y1;
  Eigen::ArrayXf eigen_angles = (dy / dx).atan();

  std::vector<float> angles(&eigen_angles[0], eigen_angles.data()+eigen_angles.cols()*eigen_angles.rows());
  std::vector<size_t> indices(angles.size());                                                        
  std::iota(indices.begin(), indices.end(), 0);                                                      
  std::sort(indices.begin(), indices.end(), [&angles](size_t i1, size_t i2) { return angles[i1] < angles[i2]; });

  // search clusters
  float angle_thr = angle_threshold;
  float distance_thr = distance_threshold;
  float ep_thr = endpoint_threshold * endpoint_threshold;
  float quater_PI = M_PI / 4.0;

  std::vector<std::vector<size_t>> cluster_ids;
  std::vector<int> cluster_codes(source_line_num, -1);
  std::vector<float> delta_angles(source_line_num, FLT_MAX);
  std::vector<bool> sort_by_x;
  for(size_t i = 0; i < source_line_num; i++){
    size_t idx1 = indices[i];
    float x11 = source_lines[idx1](0);
    float y11 = source_lines[idx1](1);
    float x12 = source_lines[idx1](2);
    float y12 = source_lines[idx1](3);
    float angle1 = angles[idx1];
    bool to_sort_x = (std::abs(angle1) < quater_PI);
    sort_by_x.push_back(to_sort_x);
    if((to_sort_x && (x12 < x11)) || ((!to_sort_x) && y12 < y11)){
      std::swap(x11, x12);
      std::swap(y11, y12);
    }

    int cluster_code = cluster_codes[i];
    if(cluster_code < 0){
      cluster_codes[i] = (int)cluster_ids.size();
      std::vector<size_t> cluster;
      cluster.push_back(idx1);
      cluster_ids.push_back(cluster);
      cluster_code = cluster_codes[i];
    }

    for(size_t j = i +1; j < source_line_num; j++){
      size_t idx2 = indices[j];
      float x21 = source_lines[idx2](0);
      float y21 = source_lines[idx2](1);
      float x22 = source_lines[idx2](2);
      float y22 = source_lines[idx2](3);
      if((to_sort_x && (x22 < x21)) || ((!to_sort_x) && y22 < y21)){
        std::swap(x21, x22);
        std::swap(y21, y22);
      }

      // check delta angle
      float angle2 = angles[idx2];
      float d_angle = AngleDiff(angle1, angle2);
      std::cout << "i = " << i << " j = " << j << ", merge" << std::endl;
      std::cout << "idxi = " << idx1 << " idxj = " << idx2 << std::endl; 
      std::cout << "linei  = " << source_lines[idx1].transpose() << std::endl;
      std::cout << "linej  = " << source_lines[idx2].transpose() << std::endl;
      std::cout << "anglei = " << angle1 << " anglej = " << angle2 << " d_angle = " << d_angle << std::endl;
      if(d_angle > angle_thr){
        if(std::abs(angle1) < (M_PI_2 - angle_threshold)){
          break;
        }else{
          continue;
        }
      }

     // check cluster code
      std::function<void(size_t&)> UpdateDeltaData = [&](size_t& update_id){
        if(d_angle < delta_angles[update_id]){
          delta_angles[update_id] = d_angle;
        }
      };
      if(cluster_codes[j] == cluster_code){
        UpdateDeltaData(i);
        UpdateDeltaData(j);
        continue;
      }

      float d1 = PointLineDistance(source_lines[idx1], source_lines[idx2].head(2));
      float d2 = PointLineDistance(source_lines[idx1], source_lines[idx2].tail(2));
      float d3 = PointLineDistance(source_lines[idx2], source_lines[idx1].head(2));
      float d4 = PointLineDistance(source_lines[idx2], source_lines[idx1].tail(2));
      std::cout << "d1 = " << d1 << " d2 = " << d2 << " d3 = " << d3 << " d4 = " << d4 << std::endl;

      // check distance
      if(PointLineDistance(source_lines[idx1], source_lines[idx2].head(2)) > distance_thr) continue;
      if(PointLineDistance(source_lines[idx1], source_lines[idx2].tail(2)) > distance_thr) continue;
      if(PointLineDistance(source_lines[idx2], source_lines[idx1].head(2)) > distance_thr) continue;
      if(PointLineDistance(source_lines[idx2], source_lines[idx1].tail(2)) > distance_thr) continue;

      // check endpoints distance
      float cx12, cy12, cx21, cy21;
      if((to_sort_x && x12 > x22) || (!to_sort_x && y12 > y22)){
        cx12 = x22;
        cy12 = y22;
        cx21 = x11;
        cy21 = y11;
      }else{
        cx12 = x12;
        cy12 = y12;
        cx21 = x21;
        cy21 = y21;
      }
      bool to_merge = ((to_sort_x && cx12 >= cx21) || (!to_sort_x && cy12 >= cy21));
      if(!to_merge){
        float d_ep = (cx21 - cx12) * (cx21 - cx12) + (cy21 - cy12) * (cy21 - cy12);
        to_merge = (d_ep < ep_thr);
      }


      if(to_merge){
        UpdateDeltaData(i);
        UpdateDeltaData(j);
        cluster_ids[cluster_code].push_back(idx2);
        cluster_codes[j] = cluster_code;
      }
    }
  }

  // merge clusters
  dst_lines.clear();
  dst_lines.reserve(cluster_ids.size());
  for(auto& cluster : cluster_ids){
    size_t idx0 = cluster[0];
    std::cout << "------------ new cluster --------" << std::endl;
    Eigen::Vector4f new_line = source_lines[idx0];
    std::cout << "idx = " << idx0 << " line = " << new_line.transpose() << std::endl;
    for(size_t i = 1; i < cluster.size(); i++){
      new_line = MergeTwoLines(new_line, source_lines[cluster[i]]);
      std::cout << "idx = " << cluster[i] << " line = " << source_lines[cluster[i]].transpose() << std::endl;
    }
    std::cout << "new line  = " << new_line.transpose() << std::endl;
    std::cout << "------------ new cluster end --------" << std::endl;
    dst_lines.push_back(new_line);
  }
}


void LineDetector::MergeLinesOld(std::vector<Eigen::Vector4f>& source_lines, std::vector<Eigen::Vector4f>& dst_lines,
    float angle_threshold, float distance_threshold, float endpoint_threshold, float theta_threshold){

  size_t source_line_num = source_lines.size();
  Eigen::Array4Xf line_array = Eigen::Map<Eigen::Array4Xf, Eigen::Unaligned>(source_lines[0].data(), 4, source_lines.size());
  Eigen::ArrayXf x1 = line_array.row(0);
  Eigen::ArrayXf y1 = line_array.row(1);
  Eigen::ArrayXf x2 = line_array.row(2);
  Eigen::ArrayXf y2 = line_array.row(3);

  Eigen::ArrayXf dx = x2 - x1;
  Eigen::ArrayXf dy = y2 - y1;
  Eigen::ArrayXf dot = (x1 * y2 - x2 * y1).abs();
  Eigen::ArrayXf eigen_distances = dot / (dx * dx + dy * dy).sqrt();
  Eigen::ArrayXf eigen_angles = (dy / dx).atan();

  std::vector<float> distances(&eigen_distances[0], eigen_distances.data()+eigen_distances.cols()*eigen_distances.rows());
  std::vector<float> angles(&eigen_angles[0], eigen_angles.data()+eigen_angles.cols()*eigen_angles.rows());

  std::vector<size_t> indices(distances.size());                                                        
  std::iota(indices.begin(), indices.end(), 0);                                                      
  std::sort(indices.begin(), indices.end(), [&distances](size_t i1, size_t i2) { return distances[i1] > distances[i2]; });

  // search clusters
  float theta_upper_thr = cos((theta_threshold));
  float theta_lower_thr = cos((M_PI - theta_threshold));
  float angle_thr = angle_threshold;
  float distance_thr = distance_threshold;
  float ep_thr = endpoint_threshold * endpoint_threshold;
  float quater_PI = M_PI / 4.0;

  std::vector<std::vector<size_t>> cluster_ids;
  std::vector<int> cluster_codes(source_line_num, -1);
  std::vector<float> delta_angles(source_line_num, FLT_MAX);
  std::vector<float> delta_distances(source_line_num, FLT_MAX);
  std::vector<bool> sort_by_x;
  for(size_t i = 0; i < source_line_num; i++){
    size_t idx1 = indices[i];
    float x11 = source_lines[idx1](0);
    float y11 = source_lines[idx1](1);
    float x12 = source_lines[idx1](2);
    float y12 = source_lines[idx1](3);
    float distance1 = distances[idx1];
    float angle1 = angles[idx1];
    bool to_sort_x = (std::abs(angle1) < quater_PI);
    sort_by_x.push_back(to_sort_x);
    if((to_sort_x && (x12 < x11)) || ((!to_sort_x) && y12 < y11)){
      std::swap(x11, x12);
      std::swap(y11, y12);
    }

    int cluster_code = cluster_codes[i];
    if(cluster_code < 0){
      cluster_codes[i] = (int)cluster_ids.size();
      std::vector<size_t> cluster;
      cluster.push_back(idx1);
      cluster_ids.push_back(cluster);
      cluster_code = cluster_codes[i];
    }

    for(size_t j = i +1; j < source_line_num; j++){
      // check delta distance
      size_t idx2 = indices[j];
      float distance2 = distances[idx2];
      float angle2 = angles[idx2];
      float d_distance = std::abs(distance2 - distance1);
      if(d_distance > distance_thr) break;

      // check delta angle
      float d_angle_case1 = std::abs(angle2 - angle1);
      float d_angle_case2 = M_PI + std::min(angle1, angle2) - std::max(angle1, angle2);
      float d_angle = std::min(d_angle_case1, d_angle_case2);
      if(d_angle > angle_thr) continue;

     // check cluster code
      std::function<void(size_t&)> UpdateDeltaData = [&](size_t& update_id){
        if(d_angle < delta_angles[update_id] && d_distance < delta_distances[update_id]){
          delta_angles[update_id] = d_angle;
          delta_distances[update_id] = d_distance;
        }
      };
      if(cluster_codes[j] == cluster_code){
        UpdateDeltaData(i);
        UpdateDeltaData(j);
        continue;
      }

      // check endpoints vector angles
      float x21 = source_lines[idx2](0);
      float y21 = source_lines[idx2](1);
      float x22 = source_lines[idx2](2);
      float y22 = source_lines[idx2](3);
      if((to_sort_x && (x22 < x21)) || ((!to_sort_x) && y22 < y21)){
        std::swap(x21, x22);
        std::swap(y21, y22);
      }

      bool to_merge = true;
      Eigen::Vector2f v21_11 = source_lines[idx2].head(2) - source_lines[idx1].head(2);
      Eigen::Vector2f v21_12 = source_lines[idx2].head(2) - source_lines[idx1].tail(2);
      Eigen::Vector2f v22_11 = source_lines[idx2].tail(2) - source_lines[idx1].head(2);
      Eigen::Vector2f v22_12 = source_lines[idx2].tail(2) - source_lines[idx1].tail(2);

      float v21_11_norm = v21_11.norm();
      float v21_12_norm = v21_12.norm();
      float v22_11_norm = v22_11.norm();
      float v22_12_norm = v22_12.norm();

      float cos_theta11 = (float)(v21_11.transpose() * v22_11) / (v21_11_norm * v22_11_norm);
      float cos_theta12 = (float)(v21_12.transpose() * v22_12) / (v21_12_norm * v22_12_norm);
      float cos_theta21 = (float)(v21_11.transpose() * v21_12) / (v21_11_norm * v21_12_norm);
      float cos_theta22 = (float)(v22_11.transpose() * v22_12) / (v22_11_norm * v22_12_norm);

      to_merge = to_merge && (cos_theta11 > theta_upper_thr || cos_theta11 < theta_lower_thr);
      to_merge = to_merge && (cos_theta12 > theta_upper_thr || cos_theta12 < theta_lower_thr);
      to_merge = to_merge && (cos_theta21 > theta_upper_thr || cos_theta21 < theta_lower_thr);
      to_merge = to_merge && (cos_theta22 > theta_upper_thr || cos_theta22 < theta_lower_thr);

      // std::cout << "(" << x11 << "," << y11 << "), " << "(" << x12 << "," << y12 << ") --- "
      //           << "(" << x21 << "," << y21 << "), " << "(" << x22 << "," << y22 << ")" << std::endl;

      // std::cout << "theta_upper_thr = " << theta_upper_thr << " theta_lower_thr = " << theta_lower_thr << std::endl;
      // std::cout << " cos_theta11 = " << cos_theta11 
      //           << " cos_theta12 = " << cos_theta12 
      //           << " cos_theta21 = " << cos_theta21 
      //           << " cos_theta22 = " << cos_theta22 << std::endl;
      // if(to_merge) std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;

      if(!to_merge) continue;

      // check endpoints distance
      float cx12, cy12, cx21, cy21;
      if((to_sort_x && x12 > x22) || (!to_sort_x && y12 > y22)){
        cx12 = x22;
        cy12 = y22;
        cx21 = x11;
        cy21 = y11;
      }else{
        cx12 = x12;
        cy12 = y12;
        cx21 = x21;
        cy21 = y21;
      }
      to_merge = ((to_sort_x && cx12 >= cx21) || (!to_sort_x && cy12 >= cy21));
      if(!to_merge){
        float d_ep = (cx21 - cx12) * (cx21 - cx12) + (cy21 - cy12) * (cy21 - cy12);
        to_merge = (d_ep < ep_thr);
      }

      // check point-line distance
      if(to_merge){
        int short_distance_num = 0;
        if(PointLineDistance(source_lines[idx1], source_lines[idx2].head(2)) < 5) short_distance_num++;
        if(PointLineDistance(source_lines[idx1], source_lines[idx2].tail(2)) < 5) short_distance_num++;
        if(PointLineDistance(source_lines[idx2], source_lines[idx1].head(2)) < 5) short_distance_num++;
        if(PointLineDistance(source_lines[idx2], source_lines[idx1].tail(2)) < 5) short_distance_num++;
        to_merge = (short_distance_num >= 2);
      }

      // std::cout << "to_merge = " << to_merge 
      //           << " i = " << i << " distance1 = " << distance1 << " angle1 = " << angle1 
      //           << " j = " << j << " distance2 = " << distance2 << " angle2 = " << angle2 
      //           << " d_distance = " << d_distance << " d_angle = " << d_angle << std::endl << std::endl << std::endl;

      if(to_merge){
        UpdateDeltaData(i);
        UpdateDeltaData(j);
        cluster_ids[cluster_code].push_back(idx2);
        cluster_codes[j] = cluster_code;
      }
    }
  }

  // merge clusters
  dst_lines.clear();
  dst_lines.reserve(cluster_ids.size());
  for(size_t i = 0; i < cluster_ids.size(); i++){
    float min_x = FLT_MAX;
    float min_y = FLT_MAX;
    float max_x = -1;
    float max_y = -1;
    // std::cout << "new cluster i = " << i << std::endl;
    // std::cout << "sort_by_x[i] = " << sort_by_x[i] << std::endl;
    for(auto& idx : cluster_ids[i]){
      float x1 = source_lines[idx](0);
      float y1 = source_lines[idx](1);
      float x2 = source_lines[idx](2);
      float y2 = source_lines[idx](3);

      // if overlap is too big
      {
        float o1, o2, n1, n2;
        if(sort_by_x[i]){
          o1 = min_x;
          o2 = max_x;
          n1 = std::min(x1, x2);
          n2 = std::max(x1, x2);
        }else{
          o1 = min_y;
          o2 = max_y;
          n1 = std::min(y1, y2);
          n2 = std::max(y1, y2);       
        }
        float overlap_length, overlap_rate;
        if((o2 > n2 && n2 > o1 && o1 > n1) || (n2 > o2 && o2 > n1 && n1 >o1)){
          float overlap_length = std::min(o2, n2) - std::max(o1, n1);
          float delta_o = o2 - o1;
          float delta_n = n2 - n1;
          float overlap_rate = overlap_length / std::min(delta_o, delta_n);
          if(overlap_rate > 0.7){
            if(delta_n > delta_o){
              if((sort_by_x[i] && x2 > x1) || (!sort_by_x[i] && y2 > y1)){
                min_x = x1;
                min_y = y1;
                max_x = x2;
                max_y = y2;
              }else{
                min_x = x2;
                min_y = y2;
                max_x = x1;
                max_y = y1;
              }
            }
            continue;
          }
        }
      }

      // std::cout << "x1 = " << x1 << " y1 = " << y1 << " x2 = " << x2 << " y2 = " << y2 << std::endl;
      MinXY(min_x, min_y, x1, y1, sort_by_x[i]);
      // std::cout << "min_x = " << min_x << " min_y = " << min_y << " max_x = " << max_x << " max_y = " << max_y << std::endl;
      MinXY(min_x, min_y, x2, y2, sort_by_x[i]);
      // std::cout << "min_x = " << min_x << " min_y = " << min_y << " max_x = " << max_x << " max_y = " << max_y << std::endl;
      MaxXY(max_x, max_y, x1, y1, sort_by_x[i]);
      // std::cout << "min_x = " << min_x << " min_y = " << min_y << " max_x = " << max_x << " max_y = " << max_y << std::endl;
      MaxXY(max_x, max_y, x2, y2, sort_by_x[i]);
      // std::cout << "min_x = " << min_x << " min_y = " << min_y << " max_x = " << max_x << " max_y = " << max_y << std::endl;
    }
    // std::cout << "---------------new cluster end -----------" << std::endl;
    dst_lines.emplace_back(min_x, min_y, max_x, max_y);
  }
}

void LineDetector::MinXY(float& min_x, float& min_y, float& x, float& y, bool to_sort_x){
  if((to_sort_x && min_x > x) || (!to_sort_x && min_y > y)){
    min_x = x;
    min_y = y;
  }
}

void LineDetector::MaxXY(float& max_x, float& max_y, float& x, float& y, bool to_sort_x){
  if((to_sort_x && max_x < x) || (!to_sort_x && max_y < y)){
    max_x = x;
    max_y = y;
  }
}

float LineDetector::PointLineDistance(Eigen::Vector4f line, Eigen::Vector2f point){
  float x0 = point(0);
  float y0 = point(1);
  float x1 = line(0);
  float y1 = line(1);
  float x2 = line(2);
  float y2 = line(3);
  float d = (std::fabs((y2 - y1) * x0 +(x1 - x2) * y0 + ((x2 * y1) -(x1 * y2)))) / (std::sqrt(std::pow(y2 - y1, 2) + std::pow(x1 - x2, 2)));
  return d;
}