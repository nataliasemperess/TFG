#include <gazebo/common/Plugin.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo_ros/node.hpp>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

namespace gazebo
{
  class LiftPlugin : public ModelPlugin
  {
  public:
    void Load(physics::ModelPtr model, sdf::ElementPtr) override
    {
      this->model_ = model;

      node_ = gazebo_ros::Node::Get(model->GetName());
      rclcpp::QoS qos(10);
      sub_ = node_->create_subscription<std_msgs::msg::String>(
          "/ascensor_planta", qos,
          [this](std_msgs::msg::String::UniquePtr msg)
          {
            std::string planta = msg->data;
            double altura = GetAltura(planta);
            auto pos = model_->WorldPose();
            pos.Pos().Z() = altura;
            model_->SetWorldPose(pos);
          });
    }

  private:
    double GetAltura(const std::string &planta)
    {
      if (planta == "sotano") return 0.0;
      if (planta == "planta0") return 0.1;
      if (planta == "planta1") return 2.5;
      if (planta == "planta2") return 5.0;
      if (planta == "planta3") return 7.5;
      if (planta == "planta4") return 10.0;
      return 0.0;
    }

    gazebo_ros::Node::SharedPtr node_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr sub_;
    physics::ModelPtr model_;
  };

  GZ_REGISTER_MODEL_PLUGIN(LiftPlugin)
}
