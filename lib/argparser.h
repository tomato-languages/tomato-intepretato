#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <variant>

#define ARGUMENT_ALREADY_EXIST_ERROR 228
#define MULTI_FLAG_ERROR 1337
#define ARGUMENT_W_O_VALUE 1420
#define POSITIONAL_MORE_THAN_ONE 52
#define HELP_OUTED 69
#define HELP_ALREADY_EXIST_ERROR 311

namespace ArgumentParser {

void ErrorMessage(const char* msg, int err_code) {
	std::cerr << "Error. " << msg;
	exit(err_code);
}


class ArgParser;

class ArgumentCarrage {

    public:

		bool is_multi {false};
		int min_count_multi_value {-1};

		bool is_flag {false};

		bool is_positional {false};
		
		bool is_help {false};
		
		bool need_to_set {true};

		ArgumentCarrage(ArgParser &parent)
			: parent_ (&parent)
		{}

		ArgumentCarrage(ArgParser &parent, const std::string &description)
			: parent_ (&parent)
			, description_ (description)
		{}

		ArgumentCarrage(ArgParser &parent, const bool &flag, const bool &help)
			: parent_ (&parent)
			, is_flag (flag)
		{}

		ArgumentCarrage(ArgParser &parent, const bool &flag, const bool &help, const std::string &description) 
			: parent_ (&parent)
			, is_flag (flag)
			, is_help (help)
			, description_ (description)
		{}


		virtual ~ArgumentCarrage() = default;

		virtual int GetCountMultiValue() const = 0;

		virtual ArgumentCarrage& SetValue(const std::string &value) = 0;
		virtual ArgumentCarrage& SetValue(const bool &flag) = 0;
		virtual std::variant<int, std::string, bool, std::vector<std::string>, std::vector<int>> GetValue() const = 0;


    protected:
		ArgParser* parent_;

		char short_arg_ {' '};
		std::string description_ {""};

		friend class ArgParser;
};
template <typename V>
class ArgumentValue : public ArgumentCarrage {

	public:

		

		ArgumentValue() = default;

		ArgumentValue(ArgParser &parent, const bool &flag, const bool &help) 
			: ArgumentCarrage (parent, flag, help)
		{}

		ArgumentValue(ArgParser &parent, const bool &flag, const bool &help, const std::string &description) 
			: ArgumentCarrage (parent, flag, help, description)
		{}

        ArgumentValue& SetValue(const std::string &value) override;
        ArgumentValue& SetValue(const bool &flag) override;

		ArgumentValue& Default(const V &default_value);
		ArgumentValue& Default(const std::vector<V> &default_value);

		ArgumentValue& Positional();

		ArgumentValue& MultiValue();
		ArgumentValue& MultiValue(const int& value);

		int GetCountMultiValue() const override;

		ArgumentValue& StoreValue(V &variable);
		ArgumentValue& StoreValues(std::vector<V> &variable);

		std::variant<int, std::string, bool, std::vector<std::string>, std::vector<int>> GetValue() const override;

	private:
		V value_ {};
		std::vector<V> multi_value_ {};

		V* variable_ = &value_;
		std::vector<V>* p_vector_ = &multi_value_;

};
class ArgParser {
    
	public:

		ArgParser() = default;

        ArgParser(const std::string &parser_name) 
            : parser_name_ (parser_name)
        {}

		~ArgParser() {
			for (auto it = arguments_.begin(); it != arguments_.end(); it++) {
				delete it->second;
			}
		}
		
		bool Parse(int argc, char** argv);
		bool Parse(const std::vector<std::string> &argv);

		ArgumentValue<std::string>& AddStringArgument(const std::string &long_arg);
		ArgumentValue<std::string>& AddStringArgument(const char &short_arg, const std::string &long_arg);
		ArgumentValue<std::string>& AddStringArgument(const char &short_arg, const std::string &long_arg, const std::string &description);

		ArgumentValue<int>& AddIntArgument(const std::string &long_arg);
		ArgumentValue<int>& AddIntArgument(const std::string &long_arg, const std::string &description);
		ArgumentValue<int>& AddIntArgument(const char &short_arg, const std::string &long_arg);
		ArgumentValue<int>& AddIntArgument(const char &short_arg, const std::string &long_arg, const std::string &description);

		ArgumentValue<bool>& AddFlag(const std::string &long_arg);
		ArgumentValue<bool>& AddFlag(const std::string &long_arg, const std::string &description);
		ArgumentValue<bool>& AddFlag(const char &short_arg, const std::string &long_arg);
		ArgumentValue<bool>& AddFlag(const char &short_arg, const std::string &long_arg, const std::string &description);

		ArgumentValue<std::string>& AddHelp(const std::string &long_arg);
		ArgumentValue<std::string>& AddHelp(const char &short_arg, const std::string &long_arg);
		ArgumentValue<std::string>& AddHelp(const char &short_arg, const std::string &long_arg, const std::string &description);

		bool Help();
		std::string HelpDescription();

		std::string GetStringValue(const std::string &long_arg);
		std::string GetStringValue(const char &short_arg);
		std::string GetStringValue(const std::string &long_arg, const int &num_of_value);
		std::string GetStringValue(const char &short_arg, const int &num_of_value);

		int GetIntValue(const std::string &long_arg);
		int GetIntValue(const char &short_arg);
		int GetIntValue(const std::string &long_arg, const int &num_of_value);
		int GetIntValue(const char &short_arg, const int &num_of_value);

		bool GetFlag(const std::string &long_arg);
		bool GetFlag(const char &short_arg) ;

	private:

		template <typename V>
		ArgumentValue<V>& AddArgument(const std::string &long_arg, const bool& is_flag, const bool& is_help);
		
		template <typename V>
		ArgumentValue<V>& AddArgument(const char &short_arg, const std::string &long_arg, const bool& is_flag, const bool& is_help);

		template <typename V>
		ArgumentValue<V>& AddArgument(const char &short_arg, const std::string &long_arg, const std::string &description, const bool& is_flag, const bool& is_help);

		template <typename V>
		ArgumentValue<V>& AddArgument(const std::string &long_arg, const std::string &description, const bool& is_flag, const bool& is_help);


		ArgumentCarrage* positional_arg_ {};
		ArgumentCarrage* help_arg_ {};

		bool need_help {false};

		std::string parser_name_;

		std::unordered_map<std::string, ArgumentCarrage*> arguments_;

		std::unordered_map<char, std::string> alias_;

		friend class ArgumentCarrage;

		template<typename V>
		friend class ArgumentValue;
};

} // namespace ArgumentParser




using namespace ArgumentParser;

template <typename V>
ArgumentValue<V>& ArgumentValue<V>::SetValue(const std::string &value)  {
    
    need_to_set = false;
    if (!is_multi) {
        if constexpr (std::is_same_v<V, int>) {
            *variable_ = stoi(value);
        } else if constexpr ((std::is_same_v<V, std::string>)) {
            *variable_ = value;
        }
    } else {
        if constexpr (std::is_same_v<V, int>) {
            p_vector_->push_back(stoi(value));
        } else if constexpr ((std::is_same_v<V, std::string>)) {
            p_vector_->push_back(value);
        }
    }
    
    return *this;
}

template <typename V>
ArgumentValue<V>& ArgumentValue<V>::SetValue(const bool &flag)  {
    need_to_set = false;
    *variable_ = flag;
    
    return *this;
}


template <typename V>
ArgumentValue<V>& ArgumentValue<V>::Default(const V &default_value) {
    need_to_set = false;
    *variable_ = default_value;

    return *this;
}

template <typename V>
ArgumentValue<V>& ArgumentValue<V>::Default(const std::vector<V> &default_value) {
    need_to_set = false;
    *p_vector_ = default_value;

    return *this;
}

template <typename V>
ArgumentValue<V>& ArgumentValue<V>::Positional() {
    if (parent_->positional_arg_ != nullptr) ErrorMessage("Positional argument can be only one", POSITIONAL_MORE_THAN_ONE);

    parent_->positional_arg_ = this;
    is_positional = true;

    return *this;
}

template <typename V>
ArgumentValue<V>& ArgumentValue<V>::MultiValue()  {
    if (is_flag) ErrorMessage("Flag can't be multi", MULTI_FLAG_ERROR);

    is_multi = true;

    return *this;
}

template <typename V>
ArgumentValue<V>& ArgumentValue<V>::MultiValue(const int& value) {
    if (is_flag) ErrorMessage("Flag can't be multi", MULTI_FLAG_ERROR);

    is_multi = true;
    min_count_multi_value = value;

    return *this;
}

template <typename V>
int ArgumentValue<V>::GetCountMultiValue() const {
    return p_vector_->size();
}


template <typename V>
ArgumentValue<V>& ArgumentValue<V>::StoreValue(V &variable) {
    variable = value_;
    variable_ = &variable;

    return *this;
}

template <typename V>
ArgumentValue<V>& ArgumentValue<V>::StoreValues(std::vector<V> &variable) {
    variable = multi_value_;
    p_vector_ = &variable;

    return *this;
}

template <typename V>
std::variant<int, std::string, bool, std::vector<std::string>, std::vector<int>> ArgumentValue<V>::GetValue() const {
    if constexpr (std::is_same_v<V, bool>) {
        return *variable_;  
    } else {
        if (is_multi) {
            return *p_vector_; 
        } else {
            return *variable_;  
        }
    }    
}



bool ArgParser::Parse(int argc, char** argv) {

    for (int current = 1; current < argc; current++) {
        std::string arg = argv[current];
        size_t pos;

        if (arg.substr(0,2) == "--") {
            
            if ((pos = arg.find_first_of('=', 2)) != std::string::npos) {
                std::string param = arg.substr(2, pos-2);
                std::string value = arg.substr(pos+1, arg.length());

                if (!arguments_.contains(param)) return false;

                ArgumentCarrage* argument_carrage = arguments_[param];

                argument_carrage->SetValue(value);

            } else {							
                std::string param = arg.substr(2, arg.length()-2);

                if (!arguments_.contains(param)) return false;
                ArgumentCarrage* argument_carrage = arguments_[param];

                if(argument_carrage->is_help) {
					need_help = true;

					return true;
                }

                if (argument_carrage->is_flag) {
                    argument_carrage->SetValue(true);
                } else {
                    current++;
                    if (current >= argc || argv[current][0] == '-') return false; // ARGUMENT_W_O_VALUE

                    if (argument_carrage->is_multi) {

                        while (current < argc && argv[current][0] != '-') {
                            argument_carrage->SetValue(std::string(argv[current++]));
                        }
                        current--;

                    } else {
                        argument_carrage->SetValue(std::string(argv[current]));
                    }
                }
            }

        } else if (arg.substr(0,1) == "-") {

            if ((pos = arg.find_first_of('=', 1)) != std::string::npos) {
                for (int i = 1; i < pos; i++) {
                    std::string param = alias_[arg[i]];
                    std::string value = arg.substr(pos+1, arg.length());

                    if (!arguments_.contains(param)) return false;

                    ArgumentCarrage* argument_carrage = arguments_[param];

                    argument_carrage->SetValue(value);
                }

            } else {							
                
                for (int i = 1; i < arg.length(); i++) {

                    if (!arguments_.contains(alias_[arg[i]])) return false;

                    ArgumentCarrage* argument_carrage = arguments_[alias_[arg[i]]];

                    if(argument_carrage->is_help) {
                        need_help = true;

						return true;
                    }

                    if (!(argument_carrage->is_flag) && i > 1) return false;

                    if (argument_carrage->is_flag) {
                        argument_carrage->SetValue(true);
                    } else {

                        current++;
                        if (current >= argc || argv[current][0] == '-') return false; // ARGUMENT_W_O_VALUE

                        if (argument_carrage->is_multi) {
                            while (current < argc && argv[current][0] != '-') {
                                argument_carrage->SetValue(std::string(argv[current++]));
                            }
                            current--;
                        } else {

                            argument_carrage->SetValue(std::string(argv[current]));
                        }
                    }

                }
            }

        } else { // Positional if exist
            if (positional_arg_ != nullptr) {
                if (positional_arg_->is_multi) {
                    while (current < argc && argv[current][0] != '-') {
                        positional_arg_->SetValue(std::string(argv[current++]));
                    }
                    current--;
                } else {

                    positional_arg_->SetValue(std::string(argv[current]));
                }
                
            } else {
                return false;
            }
        }
        
    }

    // MinCountMultiValue if
    for (auto it = arguments_.begin(); it != arguments_.end(); it++) {
        if ((it->second->is_multi && it->second->GetCountMultiValue() < it->second->min_count_multi_value) || it->second->need_to_set) return false;
    }

    return true;
}

bool ArgParser::Parse(const std::vector<std::string> &argv) {
    int argc = argv.size();

    for (int current = 1; current < argc; current++) {
        std::string arg = argv[current];
        size_t pos;

        if (arg.substr(0,2) == "--") {
            
            if ((pos = arg.find_first_of('=', 2)) != std::string::npos) {
                std::string param = arg.substr(2, pos-2);
                std::string value = arg.substr(pos+1, arg.length());

                if (!arguments_.contains(param)) return false;

                ArgumentCarrage* argument_carrage = arguments_[param];

                argument_carrage->SetValue(value);

            } else {							
                std::string param = arg.substr(2, arg.length()-2);

                if (!arguments_.contains(param)) return false;
                ArgumentCarrage* argument_carrage = arguments_[param];

                if(argument_carrage->is_help) {
					need_help = true;

					return true;
				}

                if (argument_carrage->is_flag) {
                    argument_carrage->SetValue(true);
                } else {
                    current++;
                    if (current >= argc || argv[current][0] == '-') return false; // ARGUMENT_W_O_VALUE

                    if (argument_carrage->is_multi) {

                        while (current < argc && argv[current][0] != '-') {
                            argument_carrage->SetValue(std::string(argv[current++]));
                        }
                        current--;

                    } else {
                        argument_carrage->SetValue(std::string(argv[current]));
                    }
                }
            }

        } else if (arg.substr(0,1) == "-") {

            if ((pos = arg.find_first_of('=', 1)) != std::string::npos) {
                for (int i = 1; i < pos; i++) {
                    std::string param = alias_[arg[i]];
                    std::string value = arg.substr(pos+1, arg.length());

                    if (!arguments_.contains(param)) return false;

                    ArgumentCarrage* argument_carrage = arguments_[param];

                    argument_carrage->SetValue(value);
                }

            } else {							
                
                for (int i = 1; i < arg.length(); i++) {

                    if (!arguments_.contains(alias_[arg[i]])) return false;

                    ArgumentCarrage* argument_carrage = arguments_[alias_[arg[i]]];

                    if(argument_carrage->is_help) {
                        need_help = true;

						return true;
                    }

                    if (!(argument_carrage->is_flag) && i > 1) return false;

                    if (argument_carrage->is_flag) {
                        argument_carrage->SetValue(true);
                    } else {

                        current++;
                        if (current >= argc || argv[current][0] == '-') return false; // ARGUMENT_W_O_VALUE

                        if (argument_carrage->is_multi) {
                            while (current < argc && argv[current][0] != '-') {
                                argument_carrage->SetValue(std::string(argv[current++]));
                            }
                            current--;
                        } else {

                            argument_carrage->SetValue(std::string(argv[current]));
                        }
                    }

                }
            }

        } else { // Positional if exist
            if (positional_arg_ != nullptr) {
                if (positional_arg_->is_multi) {
                    while (current < argc && argv[current][0] != '-') {
                        positional_arg_->SetValue(std::string(argv[current++]));
                    }
                    current--;
                } else {

                    positional_arg_->SetValue(std::string(argv[current]));
                }
                
            } else {
                return false;
            }
        }
        
    }

    // MinCountMultiValue if
    for (auto it = arguments_.begin(); it != arguments_.end(); it++) {
        if ((it->second->is_multi && it->second->GetCountMultiValue() < it->second->min_count_multi_value) || it->second->need_to_set) return false;
    }

    return true;
}

// bool ArgParser::Parse(std::vector<std::string> &input_args) {
//     return Parse(input_args.size(), );
// }

ArgumentValue<std::string>& ArgParser::AddStringArgument(const std::string &long_arg) {
    return AddArgument<std::string>(long_arg, false, false);
}

ArgumentValue<std::string>& ArgParser::AddStringArgument(const char &short_arg, const std::string &long_arg) {
    return AddArgument<std::string>(short_arg, long_arg, false, false);
}

ArgumentValue<std::string>& ArgParser::AddStringArgument(const char &short_arg, const std::string &long_arg, const std::string &description) {
    return AddArgument<std::string>(short_arg, long_arg, description, false, false);
}

ArgumentValue<int>& ArgParser::AddIntArgument(const std::string &long_arg) {
    return AddArgument<int>(long_arg, false, false);
}

ArgumentValue<int>& ArgParser::AddIntArgument(const char &short_arg, const std::string &long_arg) {
    return AddArgument<int>(short_arg, long_arg, false, false);
}
ArgumentValue<int>& ArgParser::AddIntArgument(const std::string &long_arg, const std::string &description) {
    return AddArgument<int>(long_arg, description, false, false);
}

ArgumentValue<int>& ArgParser::AddIntArgument(const char &short_arg, const std::string &long_arg, const std::string &description) {
    return AddArgument<int>(short_arg, long_arg, description, false, false);
}

ArgumentValue<bool>& ArgParser::AddFlag(const std::string &long_arg) {
    return AddArgument<bool>(long_arg, true, false);
}

ArgumentValue<bool>& ArgParser::AddFlag(const std::string &long_arg, const std::string &description) {
    return AddArgument<bool>(long_arg, description, true, false);
}

ArgumentValue<bool>& ArgParser::AddFlag(const char &short_arg, const std::string &long_arg) {
    return AddArgument<bool>(short_arg, long_arg, true, false);
}

ArgumentValue<bool>& ArgParser::AddFlag(const char &short_arg, const std::string &long_arg, const std::string &description) {
    return AddArgument<bool>(short_arg, long_arg, description, true, false);
}

ArgumentValue<std::string>& ArgParser::AddHelp(const std::string &long_arg) {
    return AddArgument<std::string>(long_arg, false, true);

}

ArgumentValue<std::string>& ArgParser::AddHelp(const char &short_arg, const std::string &long_arg) {
    return AddArgument<std::string>(short_arg, long_arg, false, true);

}

ArgumentValue<std::string>& ArgParser::AddHelp(const char &short_arg, const std::string &long_arg, const std::string &description) {
    return AddArgument<std::string>(short_arg, long_arg, description, false, true);

}

std::string ArgParser::HelpDescription() {
	std::string result {};

    result += help_arg_->description_; 

    for (auto it = arguments_.begin(); it != arguments_.end(); it++) {
        if (isblank(it->second->short_arg_) && !(it->second->is_help)) {
            result += "\n\n--" + it->first + ' ' + it->second->description_;
        } else if (!isblank(it->second->short_arg_) && !(it->second->is_help)){
            result += "\n\n-" + std::string(&it->second->short_arg_, &it->second->short_arg_ + 1) + " --" + it->first + "   " + it->second->description_;
        }
    }

	return result;
}

bool ArgParser::Help() {
    return need_help;
}

std::string ArgParser::GetStringValue(const std::string &long_arg) {
    return std::get<std::string>(arguments_[long_arg]->GetValue());
}

std::string ArgParser::GetStringValue(const char &short_arg) {
    return std::get<std::string>(arguments_[alias_[short_arg]]->GetValue());
}

std::string ArgParser::GetStringValue(const std::string &long_arg, const int &num_of_value) {
    return std::get<std::vector<std::string>>(arguments_[long_arg]->GetValue())[num_of_value];
}

std::string ArgParser::GetStringValue(const char &short_arg, const int &num_of_value) {
    return std::get<std::vector<std::string>>(arguments_[alias_[short_arg]]->GetValue())[num_of_value];
}




int ArgParser::GetIntValue(const std::string &long_arg) {

    return std::get<int>(arguments_[long_arg]->GetValue());
}

int ArgParser::GetIntValue(const char &short_arg) {
    return std::get<int>(arguments_[alias_[short_arg]]->GetValue());
}

int ArgParser::GetIntValue(const std::string &long_arg, const int &num_of_value) {

    return std::get<std::vector<int>>(arguments_[long_arg]->GetValue())[num_of_value];
}

int ArgParser::GetIntValue(const char &short_arg, const int &num_of_value) {
    return std::get<std::vector<int>>(arguments_[alias_[short_arg]]->GetValue())[num_of_value];
}




bool ArgParser::GetFlag(const std::string &long_arg) {
    return std::get<bool>(arguments_[long_arg]->GetValue());
}

bool ArgParser::GetFlag(const char &short_arg) {
    return std::get<bool>(arguments_[alias_[short_arg]]->GetValue());

}



template <typename V>
ArgumentValue<V>& ArgParser::AddArgument(const std::string &long_arg, const bool& is_flag, const bool& is_help) {
    if (arguments_.contains(long_arg))
        ErrorMessage("Argument already exists", ARGUMENT_ALREADY_EXIST_ERROR);

    ArgumentValue<V>* argument = new ArgumentValue<V>(*this, is_flag, is_help);

    if (is_help) {
        if (help_arg_ != nullptr) ErrorMessage("Help argument already exist", HELP_ALREADY_EXIST_ERROR);
        else help_arg_ = argument; 
    }
    if (is_flag || is_help) argument->need_to_set = false;

    arguments_[long_arg] = argument;
    return *argument;
}

template <typename V>
ArgumentValue<V>& ArgParser::AddArgument(const char &short_arg, const std::string &long_arg, const bool& is_flag, const bool& is_help) {
    if (arguments_.contains(long_arg)) ErrorMessage("Argument already exist", ARGUMENT_ALREADY_EXIST_ERROR);

    ArgumentValue<V>* argument = new ArgumentValue<V> (*this, is_flag, is_help);

    argument->short_arg_ = short_arg;
    alias_[short_arg] = long_arg;

    if (is_help) {
        if (help_arg_ != nullptr) ErrorMessage("Help argument already exist", HELP_ALREADY_EXIST_ERROR);
        else help_arg_ = argument; 
    }
    if (is_flag || is_help) argument->need_to_set = false;

    arguments_[long_arg] = argument;
    return *argument;
}

template <typename V>
ArgumentValue<V>& ArgParser::AddArgument(const char &short_arg, const std::string &long_arg, const std::string &description, const bool& is_flag, const bool& is_help) {
    if (arguments_.contains(long_arg)) ErrorMessage("Argument already exist", ARGUMENT_ALREADY_EXIST_ERROR);

    ArgumentValue<V>* argument = new ArgumentValue<V> (*this, is_flag, is_help, description);

    argument->short_arg_ = short_arg;
    alias_[short_arg] = long_arg;

    if (is_help) {
        if (help_arg_ != nullptr) ErrorMessage("Help argument already exist", HELP_ALREADY_EXIST_ERROR);
        else help_arg_ = argument; 
    }
    if (is_flag || is_help) argument->need_to_set = false;

    arguments_[long_arg] = argument;
    return *argument;
}

template <typename V>
ArgumentValue<V>& ArgParser::AddArgument(const std::string &long_arg, const std::string &description, const bool& is_flag, const bool& is_help) {
    if (arguments_.contains(long_arg)) ErrorMessage("Argument already exist", ARGUMENT_ALREADY_EXIST_ERROR);

    ArgumentValue<V>* argument = new ArgumentValue<V> (*this, is_flag, is_help, description);

    if (is_help) {
        if (help_arg_ != nullptr) ErrorMessage("Help argument already exist", HELP_ALREADY_EXIST_ERROR);
        else help_arg_ = argument; 
    }
    if (is_flag || is_help) argument->need_to_set = false;

    arguments_[long_arg] = argument;
    return *argument;
}






