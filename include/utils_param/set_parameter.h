#ifndef SET_PARAMETER_H
#define SET_PARAMETER_H

/// COMPONENT
#include <utils_param/parameter.h>

/// SYSTEM
#include <vector>

namespace param {

class SetParameter : public Parameter
{
    friend class ParameterFactory;

public:
    typedef boost::any variant;

public:
    typedef boost::shared_ptr<SetParameter> Ptr;

public:
    SetParameter();
    explicit SetParameter(const std::string& name, const ParameterDescription &description);
    virtual ~SetParameter();

    virtual int ID() const { return 0x006; }
    virtual std::string TYPE() const { return "set"; }

    virtual const std::type_info &type() const;
    virtual std::string toStringImpl() const;

    void doSetValueFrom(const Parameter& other);
    void doClone(const Parameter& other);

    void doSerialize(YAML::Node& e) const;
    void doDeserialize(const YAML::Node& n);

    template <typename T>
    T def() const { return read<T>(def_); }

    template <typename T>
    void setSet(const std::vector< std::pair<std::string, T> >& set) {
        set_.clear();
        for(typename std::vector< std::pair<std::string, T> >::const_iterator it = set.begin(); it != set.end(); ++it) {
            set_[it->first] = it->second;
        }
    }

    template <typename T>
    void setSet(const std::map<std::string, T>& set) {
        set_.clear();
        for(typename std::map<std::string, T>::const_iterator it = set.begin(); it != set.end(); ++it) {
            set_[it->first] = it->second;
        }
        scope_changed(this);
    }

    void setSet(const std::vector<std::string>& set);

    void setByName(const std::string& name);

    std::string getText(int idx) const;
    std::string getText() const;

    int noParameters() const;

protected:
    virtual boost::any get_unsafe() const;
    virtual void set_unsafe(const boost::any& v);

    std::string convertToString(const variant& v) const;

private:
    template <typename T>
    T read(const variant& var) const
    {
        try {
            return boost::any_cast<T> (var);

        } catch(const boost::bad_any_cast& e) {
            throw std::logic_error(std::string("typeof SetParameter is not ") + typeid(T).name() + ": " + e.what());
        }
    }

private:
    variant value_;
    std::string txt_;
    std::map<std::string, variant> set_;
    variant def_;
};

}

#endif // SET_PARAMETER_H
