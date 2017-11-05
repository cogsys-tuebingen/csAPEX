/// HEADER
#include <csapex_client.h>

/// PROJECT
#include <csapex/core/csapex_core.h>
#include <csapex/core/settings/settings_local.h>
#include <csapex/io/server.h>
#include <csapex/io/session_client.h>
#include <csapex/model/graph_facade_local.h>
#include <csapex/msg/generic_vector_message.hpp>
#include <csapex/param/parameter_factory.h>
#include <csapex/utility/error_handling.h>
#include <csapex/utility/exceptions.h>
#include <csapex/utility/thread.h>
#include <csapex/view/csapex_view_core_local.h>
#include <csapex/view/csapex_view_core_remote.h>
#include <csapex/view/csapex_window.h>
#include <csapex/view/gui_exception_handler.h>

/// SYSTEM
#include <iostream>
#include <QtGui>
#include <QStatusBar>
#include <QMessageBox>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

using namespace csapex;

CsApexGuiApp::CsApexGuiApp(int& argc, char** argv, ExceptionHandler &handler)
    : QApplication(argc, argv), handler(handler)
{}

CsApexCoreApp::CsApexCoreApp(int& argc, char** argv, ExceptionHandler &handler)
    : QCoreApplication(argc, argv), handler(handler)
{}

bool CsApexCoreApp::notify(QObject* receiver, QEvent* event) {
    try {
        return QCoreApplication::notify(receiver, event);

    } catch(...) {
        std::exception_ptr eptr = std::current_exception();
        handler.handleException(eptr);
        return true;
    }
}

bool CsApexGuiApp::notify(QObject* receiver, QEvent* event) {
    try {
        return QApplication::notify(receiver, event);

    } catch(...) {
        std::exception_ptr eptr = std::current_exception();
        handler.handleException(eptr);
        return true;
    }
}


Main::Main(QCoreApplication* a, Settings& settings, ExceptionHandler& handler)
    : app(a), settings(settings), handler(handler), splash(nullptr)
{
    csapex::thread::set_name("cs::APEX main");
}

Main::~Main()
{
    delete splash;
}

int Main::runImpl()
{
    csapex::error_handling::init();

    int result = app->exec();

    return result;
}

int Main::runWithGui()
{
    app->processEvents();
//    SessionClient ("localhost", 12345);
    CsApexViewCoreRemote main(std::make_shared<SessionClient>("localhost", 12345));
    return 0;

//    CsApexViewCore& view_core = main;


//    CsApexWindow w(view_core);
//    w.setWindowIcon(QIcon(":/apex_logo_client.png"));
//    QObject::connect(&w, SIGNAL(statusChanged(QString)), this, SLOT(showMessage(QString)));

//    app->connect(&w, &CsApexWindow::closed, app, &QCoreApplication::quit);
//    app->connect(app, SIGNAL(lastWindowClosed()), app, SLOT(quit()));

//    csapex::error_handling::stop_request().connect([this](){
//        static int request = 0;
//        if(request == 0) {
//            raise(SIGTERM);
//        }

//        ++request;
//    });

//    w.start();

//    w.show();
//    splash->finish(&w);

//    int res = runImpl();

//    return res;
}

int Main::run()
{
    splash = new CsApexSplashScreen;
    splash->show();

    showMessage("loading libraries");

    return runWithGui();
}

void Main::showMessage(const QString& msg)
{
    if(splash->isVisible()) {
        splash->showMessage(msg);
    }
    app->processEvents();
}


int main(int argc, char** argv)
{
    SettingsLocal settings;

    int effective_argc = argc;
    std::string path_to_bin(argv[0]);

    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "show help message")
            ;
    po::positional_options_description p;

    std::shared_ptr<ExceptionHandler> handler;

    // filters all qt parameters from argv
    std::shared_ptr<GuiExceptionHandler> h(new GuiExceptionHandler(false));

    handler = h;
    QCoreApplication* app = new CsApexGuiApp(effective_argc, argv, *handler);

    h->moveToThread(app->thread());

    // filters ros remappings
    std::vector<std::string> remapping_args;
    std::vector<std::string> rest_args;
    for(int i = 1; i < effective_argc; ++i) {
        std::string arg(argv[i]);
        if(arg.find(":=") != std::string::npos)  {
            remapping_args.push_back(arg);
        } else {
            rest_args.push_back(arg);
        }
    }

    // now check for remaining parameters
    po::variables_map vm;
    std::vector<std::string> additional_args;

    try {
        po::parsed_options parsed = po::command_line_parser(rest_args).options(desc).positional(p).run();

        po::store(parsed, vm);

        po::notify(vm);

        additional_args = po::collect_unrecognized(parsed.options, po::include_positional);

    } catch(const std::exception& e) {
        std::cerr << "cannot parse parameters: " << e.what() << std::endl;
        return 4;
    }

    // add ros remappings
    additional_args.insert(additional_args.end(), remapping_args.begin(), remapping_args.end());


    // display help?
    if(vm.count("help")) {
        std::cerr << desc << std::endl;
        return 1;
    }

    // start the app
    Main m(app, settings, *handler);
    try {
        return m.run();

    } catch(const csapex::Failure& af) {
        std::cerr << af.what() << std::endl;
        return 42;
    }
}

/// MOC
#include "moc_csapex_client.cpp"
