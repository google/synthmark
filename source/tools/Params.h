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

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>


#define PARAM_MIN(a,b) ((a) < (b) ? (a) : (b))
#define PARAM_MAX(a,b) ((a) > (b) ? (a) : (b))

#define END_LINE "\n"

template <class T>
class ParamListItem {
    T mValue;
    std::string mName;
public:
    ParamListItem(const T & value, const std::string & name) {
        mValue = value;
        mName = name;
    }

    T getValue() { return mValue; }
    std::string getName() { return mName; }
};

template <class T>
class ParamList {
    std::vector<ParamListItem<T>> mList;
public:
    ParamList() {
        mCurrentIndex = -1;
        mDefaultIndex = -1;
    }

    void addParamItem(ParamListItem<T> &param) {
        mList.push_back(param);
    }

    void addParamItem(T &value,const std::string &name) {
        ParamListItem<T> param(value, name);
        addParamItem(param);
    }

    void addParams(std::vector<T> *pValues, std::vector<std::string> *pNames) {
        if (pValues != NULL) {
            for (int i = 0; i < pValues->size(); i++) {
                if (pNames != NULL && i < pNames->size()) {
                    addParamItem(pValues->at(i), pNames->at(i));
                } else {
                    //convert to string...
                    std::stringstream ss;
                    ss << pValues->at(i);
                    addParamItem(pValues->at(i), ss.str());
                }
            }
        }
    }

    ParamListItem<T>* getParamItem(int index) {
        if (index >= 0 && index < mList.size()) {
            return &(mList[index]);
        }
        return NULL;
    }

    size_t size() {
        return mList.size();
    }

    T getValueForIndex(int index) {
        if (index >= 0 && index < mList.size()) {
            return mList[index].getValue();
        }
        return T();
    }

    std::string getNameForIndex(int index) {
        if (index >= 0 && index < mList.size()) {
            return mList[index].getName();
        }
        return "";
    }

    int getIndexForValue(T value) {
        for (int i = 0; i < mList.size(); i++) {
            if (value <= mList[i].getValue() || i == mList.size() - 1) {
                return i;
            }
        }
        return -1;
    }

    int getCurrentIndex() {
        return mCurrentIndex;
    }

    int getDefaultIndex() {
        return mDefaultIndex;
    }

    void setCurrentIndex(int currentIndex) {
        if (currentIndex > -2 && currentIndex < mList.size()) {
            mCurrentIndex = currentIndex;
        }
    }

    void setDefaultIndex(int defaultIndex) {
        if (defaultIndex > -2 && defaultIndex < mList.size()) {
            mDefaultIndex = defaultIndex;
        }
    }
private:
    int mCurrentIndex;
    int mDefaultIndex;
};


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
        mHoldType = source.mHoldType;
        return *this;
    }

    virtual std::string getValueAsString() = 0;
    virtual void setValueFromString(const std::string & value) = 0;
    virtual void setDefaultValue() = 0;

    typedef enum {
        PARAM_UNDEF = -1,
        PARAM_INTEGER = 0,
        PARAM_FLOAT = 1,
    } param_base_types;

    typedef enum {
        PRINT_ALL = 0,
        PRINT_COMPACT = 1,
    } param_print_level;

    typedef enum {
        PARAM_HOLD_RANGE = 0,
        PARAM_HOLD_LIST = 1,
    } param_hold_type;

    std::string getName() {
        return mName;
    }

    std::string getDescription() {
        return mDescription;
    }

    int getType() {
        return mType;
    }

    int getHoldType() {
        return mHoldType;
    }

    virtual int getListSize() = 0;
    virtual int getListCurrentIndex() = 0;
    virtual void setListCurrentIndex(int index) = 0;
    virtual int getListDefaultIndex() = 0;

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
    ParamBase(const std::string &name, const std::string &description, int type, int holdType)
            : mDescription(description)
            , mName(name)
            , mType(type)
            , mHoldType(holdType) {
    }

private:
    std::string mDescription; //for display purposes
    std::string mName;   //for indexing, no spaces
    int mType;  //param type:
    int mHoldType; //param hold type.
};

//EXPERIMENT WITH TEMPLATE
//integer parameter
template <class T>
class ParamType : public ParamBase {
public:
    ParamType(const std::string &name, const std::string &description, int type, int holdType) :
    ParamBase(name, description, type, holdType) {
    }

    ParamType * clone() const {
        return new ParamType(*this);
    }
    ParamBase & operator= (const ParamBase & source) {
        if (this == &source) {
            return *this;
        }
        ParamBase::operator=(source);

        const ParamType * pSource = static_cast<const ParamType*>(&source);
        if (pSource) {
            mValue = pSource->mValue;
            mDefaultValue = pSource->mDefaultValue;
            mMin = pSource->mMin;
            mMax = pSource->mMax;
            mPList = pSource->mPList;
        }
        return *this;
    }

    ParamType & operator= (const ParamType & source) {
        if (this == &source) {
            return *this;
        }
        *this = *static_cast<const ParamBase*>(&source);
        return *this;
    }

    void setValue(T value) {
        switch (getHoldType()) {
            case PARAM_HOLD_RANGE:
                mValue = PARAM_MIN(mMax, PARAM_MAX(value, mMin));
                break;
            case PARAM_HOLD_LIST: {
                int index = mPList.getIndexForValue(value);
                setListCurrentIndex(index);
            }
            default:
                break;
        }
    }

    T getValue() { return mValue; };
    T getMax() { return mMax; };
    T getMin() { return mMin; };
    T getDefaultValue() { return mDefaultValue; }

    void setDefaultValue() {
        setValue(getDefaultValue());
    }

    //list
    int getListSize() {
        return mPList.size();
    }
    int getListCurrentIndex() {
        return mPList.getCurrentIndex();
    }

    void setListCurrentIndex(int index) {
        mPList.setCurrentIndex(index);
        mValue = mPList.getValueForIndex(index);
    }

    int getListDefaultIndex() {
        return mPList.getDefaultIndex();
    }

    void addParamToList(T value, const std::string & name) {
        mPList.addParamItem(value, name);
        //recompute min, max
        mMin = 0;
        mMax = (int)(mPList.size() - 1);
    }

    ParamListItem<T> * getParamFromList(int index) {
        return mPList.getParamItem(index);
    }

    std::string getParamNameFromList(int index) {
        std::string name;
        ParamListItem<T> *pItem = getParamFromList(index);
        if (pItem != NULL) {
            name = pItem->getName();
        }
        return name;
    }

    std::string toString(int level = PRINT_ALL) {
        std::stringstream ss;
        switch (level) {
            case PRINT_ALL: {
                ss << ParamBase::toString(level);
                ss << "Value: " << getValueAsString() << END_LINE;
                ss << "Default Value: " << getDefaultValueAsString() << END_LINE;
                int holdType = getHoldType();
                switch (holdType) {
                    case PARAM_HOLD_RANGE:
                        ss << "HoldType: RANGE" << END_LINE;
                        ss << " Min: " << mMin << END_LINE;
                        ss << " Max: " << mMax << END_LINE;
                        break;
                    case PARAM_HOLD_LIST: {
                        ss << "HoldType: LIST" << END_LINE;
                        int size = getListSize();
                        ss << " Items: " << size << END_LINE;
                        for (int i = 0; i < size; i++) {
                            ParamListItem<T> *pItem = getParamFromList(i);
                            ss << "  [" << i <<"] "<< pItem->getValue() << " , ";
                            ss << pItem->getName() << END_LINE;
                        }
                    }
                        break;
                }


            }
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
        int holdType = getHoldType();
        std::stringstream ss;
        switch (holdType) {
            default:
            case PARAM_HOLD_RANGE:
                ss << mValue;
                break;
            case PARAM_HOLD_LIST: {
                 int index = getListCurrentIndex();
                ss << getParamNameFromList(index);
            }
                break;
        }
        return ss.str();
    }

    std::string getDefaultValueAsString() {
        int holdType = getHoldType();
        std::stringstream ss;
        switch (holdType) {
            default:
            case PARAM_HOLD_RANGE:
                ss << mDefaultValue;
                break;
            case PARAM_HOLD_LIST: {
                int index = getListDefaultIndex();
                ss << getParamNameFromList(index);
            }
                break;
        }
        return ss.str();
    }
    void setValueFromString(const std::string & value) {
    }

protected:
    T mDefaultValue;
    T mValue;
    T mMax;
    T mMin;

    ParamList<T> mPList;
};

class ParamInteger : public ParamType<int> {
public:

    ParamInteger(const std::string &name, const std::string &description, int defaultValue,
                    int min = 0, int max = 100) : ParamType<int>(name, description, PARAM_INTEGER,
                                                              PARAM_HOLD_RANGE)
    {
        mDefaultValue = defaultValue;
        mMin = min;
        mMax = max;
        setValue(defaultValue);
    }

    ParamInteger(const std::string &name, const std::string &description,
                 std::vector<int> *pValues = NULL,
                 int defaultIndexValue = -1,
                 std::vector<std::string> *pNames = NULL) :
    ParamType<int>(name, description, PARAM_INTEGER, PARAM_HOLD_LIST) {
        mPList.addParams(pValues, pNames);
        mPList.setDefaultIndex(defaultIndexValue);
        mPList.setCurrentIndex(defaultIndexValue);
        mDefaultValue = mPList.getValueForIndex(defaultIndexValue);
        setValue(mDefaultValue);
    }

    void setValueFromString(const std::string & value) {
        int val = atoi(value.c_str());
        setValue(val);
    }
};

class ParamFloat : public ParamType<float> {
public:

    ParamFloat(const std::string &name,
               const std::string &description,
               float defaultValue,
               float min = 0,
               float max = 100) :
    ParamType<float>(name, description, PARAM_FLOAT, PARAM_HOLD_RANGE)
    {
        mDefaultValue = defaultValue;
        mMin = min;
        mMax = max;
        setValue(defaultValue);
    }

    ParamFloat(const std::string &name,
               const std::string &description,
               std::vector<float> *pValues = NULL,
               int defaultIndexValue = -1,
               std::vector<std::string> *pNames = NULL) :
    ParamType<float>(name, description, PARAM_FLOAT, PARAM_HOLD_LIST)
    {
        mPList.addParams(pValues, pNames);
        mPList.setDefaultIndex(defaultIndexValue);
        mPList.setCurrentIndex(defaultIndexValue);
        mDefaultValue = mPList.getValueForIndex(defaultIndexValue);
        setValue(mDefaultValue);
    }

    void setValueFromString(const std::string & value) {
        float val = atof(value.c_str());
        setValue(val);
    }
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

        for (int i = 0; i < count; i++) {
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

        if (index >= 0 && index < mParams.size()) {
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

#endif  // SYNTHMARK_PARAMS_H
