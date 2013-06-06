/*
 * generic_manager.hpp
 *
 *  Created on: Apr 2, 2013
 *      Author: buck <sebastian.buck@uni-tuebingen.de>
 */

#ifndef GENERIC_MANAGER_H
#define GENERIC_MANAGER_H

/// SYSTEM
#include <boost/signals2.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <pluginlib/class_loader.h>

#define STATIC_INIT(prefix, name, code)\
    namespace vision_evaluator { \
    class _____##prefix##name##_registrator { \
        static _____##prefix##name##_registrator instance; \
        _____##prefix##name##_registrator () {\
            { code } \
        } \
    };\
    _____##prefix##name##_registrator _____##prefix##name##_registrator::instance;\
    }

#define REGISTER_GENERIC(Manager, class_name)\
    STATIC_INIT(Manager, class_name, { \
        std::cout << "register filter instance " << #class_name << std::endl; \
        Manager::Constructor constructor; \
        constructor.name = #class_name; \
        constructor.constructor = boost::lambda::new_ptr<class_name>(); \
        vision_evaluator::Manager manager;\
        manager.registerConstructor(constructor); \
    });\
 
template <class M>
struct DefaultConstructor {
    std::string name;
    boost::function<M*()> constructor;

    typename boost::shared_ptr<M> operator()() const {
        boost::shared_ptr<M> res(constructor());
        res->setName(name);
        assert(res.get() != NULL);
        return res;
    }
    bool valid() const {
        typename boost::shared_ptr<M> res(constructor());
        return res.get() != NULL;
    }
};

template <class M, class C, class Container>
class PluginManagerImp
{
    template <class, class, class>
    friend class PluginManager;

protected:
    typedef C Constructor;
    typedef Container Constructors;

    static PluginManagerImp<M,C,Container>& instance(const std::string& full_name) {
        static PluginManagerImp<M,C,Container> i(full_name);
        return i;
    }

protected:
    typedef pluginlib::ClassLoader<M> Loader;

    PluginManagerImp(const std::string& full_name)
        : loader_(new Loader("vision_evaluator", full_name)) {
    }
    PluginManagerImp(const PluginManagerImp& rhs);
    PluginManagerImp& operator = (const PluginManagerImp& rhs);

protected:
    virtual ~PluginManagerImp() {
//        if(loader_ != NULL) {
//            delete loader_;
//            loader_ = NULL;
//        }
    }

    void registerConstructor(Constructor constructor) {
        available_classes.push_back(constructor);
    }

    void reload() {
        try {
            std::vector<std::string> classes = loader_->getDeclaredClasses();
            for(std::vector<std::string>::iterator c = classes.begin(); c != classes.end(); ++c) {
                std::cout << "load library for class " << *c << std::endl;
                loader_->loadLibraryForClass(*c);

                Constructor constructor;
                constructor.name = *c;
                constructor.constructor = boost::bind(&Loader::createUnmanagedInstance, loader_, *c);
                registerConstructor(constructor);
                std::cout << "loaded " << typeid(M).name() << " class " << *c << std::endl;
            }
        } catch(pluginlib::PluginlibException& ex) {
            ROS_ERROR("The plugin failed to load for some reason. Error: %s", ex.what());
        }
        plugins_loaded_ = true;
    }

protected:
    bool plugins_loaded_;
    Loader* loader_;

    Constructors available_classes;
};

template <class M, class C = DefaultConstructor<M>, class Container = std::vector<C> >
class PluginManager
{
protected:
    typedef PluginManagerImp<M, C, Container> Parent;

public:
    typedef typename Parent::Constructor Constructor;
    typedef typename Parent::Constructors Constructors;

    PluginManager(const std::string& full_name)
        : instance(Parent::instance(full_name))
    {}

    void registerConstructor(Constructor constructor) {
        instance.registerConstructor(constructor);
    }

    bool pluginsLoaded() const {
        return instance.plugins_loaded_;
    }

    void reload() {
        instance.reload();
    }

    const Constructors& availableClasses() const {
        return instance.available_classes;
    }
    const Constructor& availableClasses(unsigned index) const {
        return instance.available_classes[index];
    }
    const Constructor& availableClasses(const std::string& key) const {
        return instance.available_classes[key];
    }
    Constructor& availableClasses(const std::string& key) {
        return instance.available_classes[key];
    }

protected:
    Parent& instance;
};


#endif // GENERIC_MANAGER_H
