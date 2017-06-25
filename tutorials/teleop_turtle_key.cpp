#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include <turtlesim/Spawn.h>
#include <turtlesim/Kill.h>
#include <time.h>

#define KEYCODE_R 0x43 
#define KEYCODE_L 0x44
#define KEYCODE_U 0x41
#define KEYCODE_D 0x42
#define KEYCODE_Q 0x71

class TeleopTurtle
{
public:
  TeleopTurtle(std::string name);
  ~TeleopTurtle();
  void CleanUp();
  void keyLoop();

private:

  
  ros::NodeHandle nh_;
  double linear_, angular_, l_scale_, a_scale_;
  ros::Publisher twist_pub_;
  ros::ServiceClient spawn_client;
  ros::ServiceClient kill_client;
  turtlesim::Spawn client_turtle_;
  
};

TeleopTurtle::TeleopTurtle(std::string name):
  linear_(0),
  angular_(0),
  l_scale_(2.0),
  a_scale_(2.0)
{
  //Create a new turtle for this instance
  spawn_client = nh_.serviceClient<turtlesim::Spawn>("/spawn");
  kill_client = nh_.serviceClient<turtlesim::Kill>("/kill");

  srand(time(NULL));
  client_turtle_.request.x = (double)rand()/RAND_MAX * 11.0;
  client_turtle_.request.y = (double)rand()/RAND_MAX * 11.0;
  client_turtle_.request.theta = (double)rand()/RAND_MAX * 3.14;
  client_turtle_.request.name = name;

  spawn_client.call(client_turtle_);

  nh_.param("scale_angular", a_scale_, a_scale_);
  nh_.param("scale_linear", l_scale_, l_scale_);

  twist_pub_ = nh_.advertise<geometry_msgs::Twist>(client_turtle_.request.name+"/cmd_vel", 1);
}

TeleopTurtle::~TeleopTurtle()
{
  CleanUp();
}

void TeleopTurtle::CleanUp()
{
  //Kill the client's turtle
  turtlesim::Kill kill_turtle_msg;
  kill_turtle_msg.request.name = client_turtle_.request.name;
  kill_client.call(kill_turtle_msg);
}

int kfd = 0;
struct termios cooked, raw;

void quit(int sig)
{
  //teleop_turtle.CleanUp();
  (void)sig;
  tcsetattr(kfd, TCSANOW, &cooked);
  ros::shutdown();
  exit(0);
}


int main(int argc, char** argv)
{
  //Assign a unique name
  std::string userName = "";
  std::cout << "Please enter a name: ";
  std::cin >> userName;

  ros::init(argc, argv, userName+"_node");
  TeleopTurtle teleop_turtle(userName);

  signal(SIGINT,quit);

  teleop_turtle.keyLoop();
  
  teleop_turtle.CleanUp();
  
  quit(SIGINT);

  return(0);
}


void TeleopTurtle::keyLoop()
{
  char c;
  bool dirty=false;


  // get the console in raw mode                                                              
  tcgetattr(kfd, &cooked);
  memcpy(&raw, &cooked, sizeof(struct termios));
  raw.c_lflag &=~ (ICANON | ECHO);
  // Setting a new line, then end of file                         
  raw.c_cc[VEOL] = 1;
  raw.c_cc[VEOF] = 2;
  tcsetattr(kfd, TCSANOW, &raw);

  puts("Reading from keyboard");
  puts("---------------------------");
  puts("Use arrow keys to move the turtle.");
  puts("Press Q to quit.");

  for(;;)
  {
    // get the next event from the keyboard  
    if(read(kfd, &c, 1) < 0)
    {
      perror("read():");
      exit(-1);
    }

    linear_=angular_=0;
    ROS_DEBUG("value: 0x%02X\n", c);
  
    switch(c)
    {
      case KEYCODE_L:
        ROS_DEBUG("LEFT");
        angular_ = 1.0;
        dirty = true;
        break;
      case KEYCODE_R:
        ROS_DEBUG("RIGHT");
        angular_ = -1.0;
        dirty = true;
        break;
      case KEYCODE_U:
        ROS_DEBUG("UP");
        linear_ = 1.0;
        dirty = true;
        break;
      case KEYCODE_D:
        ROS_DEBUG("DOWN");
        linear_ = -1.0;
        dirty = true;
        break;
      case KEYCODE_Q:
	//Quit this loop and finish the prgm
	ROS_DEBUG("QUIT");
	return;
	break;
    }
   

    geometry_msgs::Twist twist;
    twist.angular.z = a_scale_*angular_;
    twist.linear.x = l_scale_*linear_;
    if(dirty ==true)
    {
      twist_pub_.publish(twist);    
      dirty=false;
    }
  }


  return;
}



