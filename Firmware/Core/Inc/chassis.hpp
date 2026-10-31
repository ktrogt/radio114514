/*******************************************************************************
 *  File:        chassis.hpp
 *  Module:      Chassis Inverse Kinematics
 *
 *  Summary:
 *    Inverse kinematics utility for omni-wheel and mecanum drive robots.
 *
 *  Description:
 *    This header provides a lightweight chassis model that computes
 *    per-wheel motor outputs from various intuitive input representations
 *    such as velocity vectors and angular velocity.
 *
 *    It supports 3-wheel omni, 4-wheel omni, and 4-wheel mecanum drive
 *    configurations, and allows quick evaluation of inverse kinematics
 *    without requiring detailed mechanical modeling.
 *
 *  Attention:
 *    This file is part of taku-256's personal library or SAME library.
 *    "SAME" does not mean "similar"; in Japanese it means "shark".
 *    The actual pronunciation may differ from what you expect.
 *
 *******************************************************************************/

/*******************************************************************************
 *  Copyright (C) 2025 taku-256
 *
 *  License:
 *    This file is part of the SAME library.
 *    The license of this file follows the license of SAME library.
 *    Refer to the top-level LICENSE file for details.
 *
 *  This software is provided "AS IS", without warranty of any kind.
 *  The author shall not be held liable for any damages arising from the use
 *  of this software.
 *
 *******************************************************************************/

#ifndef SAME_LIBRARY__CHASSIS_HPP_
#define SAME_LIBRARY__CHASSIS_HPP_

#if defined(__has_include)
#if __has_include("rclcpp/rclcpp.hpp")
#include "rclcpp/rclcpp.hpp"
using namespace std::chrono_literals;
using namespace std::placeholders;
#endif
#if __has_include("geometry_msgs/msg/twist.hpp")
#include "geometry_msgs/msg/twist.hpp"
#endif
#if __has_include("visualization_msgs/msg/marker_array.hpp")
#include "visualization_msgs/msg/marker_array.hpp"
#endif
#endif

/*******************************************************************************
 *
 *  *mecanum
 *
 *  0 - - - 3
 *  |       |
 *  |       |
 *  |       |
 *  1 - - - 2
 *
 *  *Omni-4
 *
 *  0 -- -- 3
 *  |       |
 *  |       |
 *  1 -- -- 2
 *
 *  *Omni-3
 *
 *  0 -- -- 2
 *   \     /
 *    \   /
 *      1
 *
 *  rotation
 * + << w >> -
 *
 *  +x : front  0deg ^^^
 *  +y : right 90deg >>>
 *
 *  All motors: CW
 *
 *******************************************************************************/

#include <math.h>
#define sc_min(x, y) ((x) < (y) ? (x) : (y))
#define sc_max(x, y) ((x) > (y) ? (x) : (y))
#define sc_clamp(x, y) ((x) > (y) ? (y) : ((x) < -(y) ? -(y) : (x)))  // max(-y,min(x,y))

typedef enum {
    Mecanum,
    Omni_4,
    Omni_3
} Chassis_type;

template <Chassis_type T>
class Chassis {
private:
    static constexpr int m_len = (T == Mecanum) ? 4 : (T == Omni_4) ? 4 : (T == Omni_3) ? 3 : 0;

    double v_motors[m_len];
    double* motors[m_len];
    double limit = 100.0;

#if defined(RCLCPP__RCLCPP_HPP_) && defined(GEOMETRY_MSGS__MSG__TWIST_HPP_) && defined(VISUALIZATION_MSGS__MSG__MARKER_ARRAY_HPP_)
    visualization_msgs::msg::MarkerArray markers;
    double vec_size = 0.5;
    std::array<std::string, m_len> vec_names;
    void (*user_callback)(void) = nullptr;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr twist_sub_;
#endif

public:
    Chassis() {
        for (int i = 0; i < m_len; ++i) {
            motors[i] = &v_motors[i];
        }

#ifdef VISUALIZATION_MSGS__MSG__MARKER_ARRAY_HPP_
        if constexpr (T == Mecanum) {
            vec_names[0] = "mecanum0";
            vec_names[1] = "mecanum1";
            vec_names[2] = "mecanum2";
            vec_names[3] = "mecanum3";
        } else if constexpr (T == Omni_4) {
            vec_names[0] = "omni0";
            vec_names[1] = "omni1";
            vec_names[2] = "omni2";
            vec_names[3] = "omni3";
        } else if constexpr (T == Omni_3) {
            vec_names[0] = "omni0";
            vec_names[1] = "omni1";
            vec_names[2] = "omni2";
        }
#endif
    }

    Chassis& set_motor(int n, double* motor) {
        if (n >= 0 && n < m_len) {
            motors[n] = motor;
        }
        return *this;
    }

    double* get_motor(int n) {
        if (n < 0 || n >= m_len) {
            return nullptr;
        }
        return motors[n];
    }

    Chassis& set_limit(double n) {
        limit = n;
        return *this;
    }

    const double R2D = 180.0 / 3.14159265358979323846;
    const double D45R = 45 / R2D;
    const double D60R = 60 / R2D;

    Chassis& calc(double theta, double power, double roll) {
        if constexpr (T == Mecanum) {
            *motors[0] = sc_clamp(-(sin(theta + D45R)) * power + roll, limit);
            *motors[1] = sc_clamp(-(sin(theta - D45R)) * power + roll, limit);
            *motors[2] = sc_clamp(+(sin(theta + D45R)) * power + roll, limit);
            *motors[3] = sc_clamp(+(sin(theta - D45R)) * power + roll, limit);
        } else if constexpr (T == Omni_4) {
            *motors[0] = sc_clamp(-(sin(theta + D45R)) * power + roll, limit);
            *motors[1] = sc_clamp(-(sin(theta - D45R)) * power + roll, limit);
            *motors[2] = sc_clamp(+(sin(theta + D45R)) * power + roll, limit);
            *motors[3] = sc_clamp(+(sin(theta - D45R)) * power + roll, limit);
        } else if constexpr (T == Omni_3) {
            *motors[0] = sc_clamp(-(sin(theta - D60R)) * power + roll, limit);
            *motors[1] = sc_clamp(-(sin(theta + D60R)) * power + roll, limit);
            *motors[2] = sc_clamp(+(cos(theta)) * power + roll, limit);
        }
        return *this;
    }

    Chassis& dcalc(double theta, double power, double roll) { return calc(theta / R2D, power, roll); }

    Chassis& cmd_vel(double vx, double vy, double w) { return calc(atan2(vy, vx), sqrt(vx * vx + vy * vy), w); }

    Chassis& stop() {
        for (int i = 0; i < m_len; ++i) {
            *motors[i] = sc_clamp(0, limit);
        }
        return *this;
    }

#if defined(RCLCPP__RCLCPP_HPP_) && defined(GEOMETRY_MSGS__MSG__TWIST_HPP_) && defined(VISUALIZATION_MSGS__MSG__MARKER_ARRAY_HPP_)
    Chassis& twist(const geometry_msgs::msg::Twist::SharedPtr msg) {
        cmd_vel(msg->linear.x, msg->linear.y, msg->angular.z);
        return *this;
    }

    Chassis& set_twist_sub_(rclcpp::Node* node, const std::string& topic_path = "cmd_vel") {
        twist_sub_ = node->create_subscription<geometry_msgs::msg::Twist>(topic_path, 1, [this](const geometry_msgs::msg::Twist::SharedPtr msg) {
            this->twist(msg);
            if (this->user_callback) {
                this->user_callback();
            }
        });
        return *this;
    }

    Chassis& set_vector_size(double new_size) {
        vec_size = new_size;
        return *this;
    }

    Chassis& change_vector_name(const std::string& name, uint8_t idx) {
        if (idx < m_len) {
            vec_names[idx] = name;
        }
        return *this;
    }

    // TODO: Write visualize method
    visualization_msgs::msg::MarkerArray* get_vectors() {
        if constexpr (T == Mecanum) {
        } else if constexpr (T == Omni_4) {
        } else if constexpr (T == Omni_3) {
        }
        return &markers;
    }

    void set_callback(void (*callback)(void)) { user_callback = callback; }
#endif
};

#endif /*SAME_LIBRARY__CHASSIS_HPP_*/

/*******************************************************************************
 End of File
*/
