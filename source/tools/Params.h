/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SYNTHMARK_PARAMS_H
#define SYNTHMARK_PARAMS_H

#define PARAM_MIN(a,b) ((a)<(b) ? (a) : (b))
#define PARAM_MAX(a,b) ((a)>(b) ? (a) : (b))

#define END_LINE "\n"

#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>


class ParamBase {
public:
    virtual ~ParamBase() {}

    virtual std::string toString(int level = PRINT_ALL) {
        std::stringstream ss;
        switch (level) {
            case PRINT_ALL:
                ss << "Name: " << mName << END_LINE;
                ss << "Description: " << mDescription << END_LINE;
                ss << "Type: " << typeToString(mType) << END_LINE;
                break;
        }

        return ss.str();
    }

    virtual ParamBase * clone() const = 0;
    virtual ParamBase & operator= (const ParamBase & source) {
        mName = source.mName;
        mDescription = source.mDescription;
        mType = source.mType;
        return *this;
    }

    virtual std::string getValueAsString() = 0;
    virtual void setValueFromString(const std::string & value) = 0;

    typedef enum {
        PARAM_UNDEF = -1,
        PARAM_INTEGER = 0,
        PARAM_FLOAT = 1,
    } param_base_types;

    typedef enum{
        PRINT_ALL = 0,
        PRINT_COMPACT = 1,
    } param_print_level;

    std::string getName() {
        return mName;
    }

    std::string getDescription() {
        return mDescription;
    }

    int getType() {
        return mType;
    }

    std::string typeToString(int type) {
        std::string result = "Undefined";

        switch(type) {
            case PARAM_INTEGER:
                result = "Integer";
                break;
            case PARAM_FLOAT:
                result = "Float";
                break;
        }
        return result;
    }

protected:
    ParamBase(const std::string &name, const std::string &description, int type) : mName(name),
        mDescription(description), mType(type) {
    }

private:
    std::string mDescription; //for display purposes
    std::string mName;   //for indexing, no spaces
    int mType;  //param type:
};

//integer parameter
class ParamInteger : public ParamBase {
public:
    ParamInteger(const std::string &name, const std::string &description, int defaultValue=0,
    int min=0, int max =100) : ParamBase(name, description, PARAM_INTEGER),
    mDefaultValue(defaultValue), mMin(min), mMax(max) {
        setValue(defaultValue);
    }

    ParamInteger * clone() const {
        return new ParamInteger(*this);
    }
    ParamBase & operator= (const ParamBase & source) {
        if (this == &source) {
            return *this;
        }
        ParamBase::operator=(source);

        const ParamInteger * pSource = static_cast<const ParamInteger*>(&source);
        if (pSource) {
            mValue = pSource->mValue;
            mDefaultValue = pSource->mDefaultValue;
            mMin = pSource->mMin;
            mMax = pSource->mMax;
        }
        return *this;
    }

    ParamInteger & operator= (const ParamInteger & source) {
        if (this == &source) {
            return *this;
        }
        *this = *static_cast<const ParamBase*>(&source);
        return *this;
    }

    void setValue(int value) {
        mValue = PARAM_MIN(mMax, PARAM_MAX(value, mMin));
    }

    int getValue() { return mValue; };
    int getMax() { return mMax; };
    int getMin() { return mMin; };
    int getDefaultValue() { return mDefaultValue; }

    std::string toString(int level = PRINT_ALL) {
        std::stringstream ss;
        switch (level) {
            case PRINT_ALL:
                ss << ParamBase::toString(level);
                ss << "Value: " << mValue << END_LINE;
                ss << "Default Value: " << mDefaultValue << END_LINE;
                ss << "Min: " << mMin << END_LINE;
                ss << "Max: " << mMax << END_LINE;
                break;
            case PRINT_COMPACT:
                ss << getDescription() <<" : ";
                ss << getValueAsString();
                if (mValue == mDefaultValue) {
                    ss <<"*";
                }
                break;
        }
        return ss.str();
    }
    std::string getValueAsString() {
        std::stringstream ss;
        ss << mValue;
        return ss.str();
    }
    void setValueFromString(const std::string & value) {
        int val = atoi(value.c_str());
        setValue(val);
    }

private:
    int mDefaultValue;
    int mValue;
    int mMax;
    int mMin;

};

//float parameter
class ParamFloat : public ParamBase {
public:
    ParamFloat(const std::string &name, const std::string &description, float defaultValue=0,
    float min=0, float max =100) : ParamBase(name, description, PARAM_FLOAT),
    mDefaultValue(defaultValue), mMin(min), mMax(max) {
        setValue(defaultValue);
    }

    ParamFloat * clone() const {
        return new ParamFloat(*this);
    }
    ParamBase & operator= (const ParamBase & source) {
        if (this == &source) {
            return *this;
        }
        ParamBase::operator=(source);

        const ParamFloat * pSource = static_cast<const ParamFloat*>(&source);
        if (pSource) {
            mValue = pSource->mValue;
            mDefaultValue = pSource->mDefaultValue;
            mMin = pSource->mMin;
            mMax = pSource->mMax;
        }
        return *this;
    }

    ParamFloat & operator= (const ParamFloat & source) {
        if (this == &source) {
            return *this;
        }
        *this = *static_cast<const ParamBase*>(&source);
        return *this;
    }

    void setValue(float value) {
        mValue = PARAM_MIN(mMax, PARAM_MAX(value, mMin));
    }

    float getValue() { return mValue; };
    float getMax() { return mMax; };
    float getMin() { return mMin; };
    float getDefaultValue() { return mDefaultValue; }

    std::string toString(int level = PRINT_ALL) {
        std::stringstream ss;
        switch (level) {
            case PRINT_ALL:
                ss << ParamBase::toString();
                ss << "Value: " << mValue << END_LINE;
                ss << "Default Value: " << mDefaultValue << END_LINE;
                ss << "Min: " << mMin << END_LINE;
                ss << "Max: " << mMax << END_LINE;
                break;
            case PRINT_COMPACT:
                ss << getDescription() <<" : ";
                ss << getValueAsString();
                if (mValue == mDefaultValue) {
                    ss <<"*";
                }
                break;
        }
        return ss.str();
    }
    std::string getValueAsString() {
        std::stringstream ss;
        ss << mValue;
        return ss.str();
    }
    void setValueFromString(const std::string & value) {
        float val = atof(value.c_str());
        setValue(val);
    }

private:
    float mDefaultValue;
    float mValue;
    float mMax;
    float mMin;
};

//===========================
// ParamsGroup
//===========================

class ParamGroup {
public:
    ParamGroup(std::string groupName) : mGroupName (groupName) {
    }

    ~ParamGroup() {
        cleanAll();
    }

    void addParam(ParamBase  *pBase) {
        if (pBase != NULL) {
            ParamBase *pBaseNew = NULL;

            int type = pBase->getType();

            switch (type) {
                case ParamBase::PARAM_INTEGER: {
                    ParamInteger * pInt = static_cast<ParamInteger*>(pBase);
                    if (pInt != NULL) {
                        pBaseNew = new ParamInteger(*pInt);
                    }
                }
                    break;
                case ParamBase::PARAM_FLOAT: {
                    ParamFloat * pFloat = static_cast<ParamFloat*>(pBase);
                    if (pFloat != NULL) {
                        pBaseNew = new ParamFloat(*pFloat);
                    }
                }
                    break;
            }

            if (pBaseNew != NULL) {
                mParams.push_back(pBaseNew);
            }

        }
    }

    void cleanAll() {
        size_t count = mParams.size();

        for (int i=0; i<count; i++) {
            ParamBase * pBase = mParams[i];
            if (pBase != NULL) {
                delete(pBase);
                mParams[i] = NULL;
            }
        }
        mParams.clear();
    }

    std::string toString(int level = ParamBase::PRINT_ALL) {
        std::stringstream ss;

        int count = mParams.size();
        switch(level) {
            case ParamBase::PRINT_ALL:
                ss << "Group Name: " << mGroupName << END_LINE;
                ss << "Param Count: " << count << END_LINE;

                for (int i = 0; i < count; i++) {
                    ParamBase * pBase = mParams[i];
                    if (pBase != NULL) {
                        ss << "-- Param " << i <<" ---" << END_LINE;
                        ss << pBase->toString(level);
                    }
                }
                break;
            case ParamBase::PRINT_COMPACT:
                ss << " Group : " << mGroupName << END_LINE;
                for (int i = 0; i < count; i++) {
                    ParamBase * pBase = mParams[i];
                    if (pBase != NULL) {
                        ss << " [" << i <<"] " << pBase->toString(level) << END_LINE;
                    }
                }


                break;
        }
        return ss.str();
    }

    int getParamCount() {
        return (int)mParams.size();
    }

    ParamBase * getParamByName(std::string name) {
        ParamBase *pBase = NULL;

        const char * str1 = name.c_str();
        for (int i = 0; i < mParams.size(); i++) {
            ParamBase *pBaseTemp = mParams[i];
            if (pBaseTemp != NULL) {
                const char * str2 = pBaseTemp->getName().c_str();
                if (strcmp(str1, str2) == 0) {
                    pBase = pBaseTemp;
                    break; //done
                }
            }
        }
        return pBase;
    }

    ParamBase * getParamByIndex(int index) {
        ParamBase *pBase = NULL;

        if (index>=0 && index < mParams.size()) {
            pBase = mParams[index];
        }
        return pBase;
    }

    //Convenience methods
    int getValueFromInt(std::string name) {
        int result = 0;

        ParamBase * pBase = getParamByName(name);
        ParamInteger * pInt = static_cast<ParamInteger*>(pBase);
        if (pInt) {
            result = pInt->getValue();
        }
        return result;
    }

    float getValueFromFloat(std::string name) {
        float result = 0;

        ParamBase * pBase = getParamByName(name);
        ParamFloat * pFloat = static_cast<ParamFloat*>(pBase);
        if (pFloat) {
            result = pFloat->getValue();
        }
        return result;
    }

private:
    std::string mGroupName;
    std::vector<ParamBase*> mParams;
};

#endif