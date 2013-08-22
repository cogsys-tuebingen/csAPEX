/// HEADER
#include <csapex_core_plugins/ros_handler.h>

/// SYSTEM
#include <QtConcurrentRun>

using namespace csapex;

ROSHandler::ROSHandler()
    : initialized_(false)
{
    initHandle(true);
}

ROSHandler& ROSHandler::instance()
{
    static ROSHandler instance;
    return instance;
}

ROSHandler::~ROSHandler()
{
    spinner_->stop();
    ros::shutdown();
}

boost::shared_ptr<ros::NodeHandle> ROSHandler::nh()
{
    return nh_;
}


void ROSHandler::initHandle(bool try_only)
{
    if(!initialized_) {
        checkMasterConnection();
    }

    if(try_only && has_connection.isRunning()) {
        std::cout << "init handle: still probing master" << std::endl;
        return;
    }

    if(has_connection.result()) {
        nh_.reset(new ros::NodeHandle("~"));
        spinner_.reset(new ros::AsyncSpinner(2));
        spinner_->start();
    } else {
        std::cout << "init handle: no ros connection" << std::endl;
    }
}


void ROSHandler::checkMasterConnection()
{
    if(!ros::isInitialized()) {
        int c = 0;
        ros::init(c, (char**) NULL, "csapex");
    }

    if(!has_connection.isRunning()) {
        has_connection = QtConcurrent::run(ros::master::check);
    }
}

bool ROSHandler::waitForConnection()
{
    has_connection.waitForFinished();
    return has_connection.result();
}

void ROSHandler::refresh()
{

    waitForConnection();

    checkMasterConnection();

    if(nh_) {
        // connection was there
        has_connection.waitForFinished();
        if(!has_connection.result()) {
            // connection no longer there
            spinner_->stop();
            nh_->shutdown();
        }
    }

    initHandle();
}