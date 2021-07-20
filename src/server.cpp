#include "server.h"

static inline HttpResponsePtr makeFailedResponse(std::string msg = "")
{
    Json::Value json;
    json["success"] = false;
    json["data"] = msg;
    auto resp = HttpResponse::newHttpJsonResponse(json);
    resp->setStatusCode(k500InternalServerError);
    return resp;
}

static inline HttpResponsePtr makeSuccessResponse(std::string msg = "")
{
    Json::Value json;
    json["success"] = true;
    json["data"] = msg;
    auto resp = HttpResponse::newHttpJsonResponse(json);
    return resp;
}

inline std::string getRandomString(size_t n) {
    std::vector<unsigned char> random(n);
    utils::secureRandomBytes(random.data(), random.size());

    // This is cryptographically safe as 256 mod 16 == 0
    static const std::string alphabets = "0123456789abcdef";
    assert(256 % alphabets.size() == 0);
    std::string randomString(n, '\0');
    for (size_t i = 0; i < n; i++) {
        randomString[i] = alphabets[random[i] % alphabets.size()];
    }
    return randomString;
}

void Server::createKernel(const HttpRequestPtr& req, HttpCallback callback) {
    std::string id = getRandomString(64);
    // TODO: if already exists then return 50x error.

    auto jsonPtr = req->jsonObject();
    if (jsonPtr == nullptr) {
        return callback(makeFailedResponse("Invalid JSON"));
    }

    const Json::Value &json = *jsonPtr;
    if (!json["source"].isString()) {
        return callback(makeFailedResponse("Empty or missing kernel string"));
    } else if (!json["type"].isUInt()) {
        return callback(makeFailedResponse("Invalid data type"));
    } else if (!json["outputs"].isArray()) {
        return callback(makeFailedResponse("Missing outputs"));
    }

    const auto source = json["source"].asString();
    const auto dataType = json["type"].asUInt();
    const auto outputsArray = json["outputs"];
    std::vector<size_t> outputs;

    if (dataType != FLOAT && dataType != UINT32) {
        return callback(makeFailedResponse("Unrecognized data type"));
    }

    outputs.reserve(outputsArray.size());

    for (int i=0; i<outputsArray.size(); i++) {
        const auto arrayItem = outputsArray[i];
        if (!arrayItem.isUInt()) {
            return callback(makeFailedResponse("Outputs is not a list of integers"));
        }
        const auto value = arrayItem.asUInt();
        outputs.push_back(value);
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    auto item = std::make_shared<KernelItem>();
    item->type = dataType;

    // TODO: is it safer to use smart pointers?
    auto kernel = std::make_shared<Kernel>();

    bool ret = kernel->compile(source);
    if (!ret) {
        return callback(makeFailedResponse("Failed to compile kernel"));
    }
    kernel->addOutputParams(outputs);

    item->kernel = std::move(*kernel);

    m_kernels.emplace(id, std::move(item));

    Json::Value res;
    res["uuid"] = id;
    res["success"] = true;
    res["data"] = "Kernel created successfully";
    callback(HttpResponse::newHttpJsonResponse(std::move(res)));
}

void Server::kernelInfo(const HttpRequestPtr&, HttpCallback callback, const std::string& id) {
    //std::lock_guard<std::mutex> lock(m_mutex);

    auto itemPtr = [this, &id]() -> std::shared_ptr<KernelItem> {
        // It is possible that the item is being removed while another
        // thread tries to look it up. The mutex here prevents that from
        // happening.
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_kernels.find(id);
        if (it == m_kernels.end()) {
            return nullptr;
        }
        return it->second;
    }();

    if (itemPtr == nullptr) {
        return callback(makeFailedResponse("Kernel not found"));
    }

    auto& item = *itemPtr;
    // Prevents another thread from writing to the same item while this
    // thread reads. Could cause blockage if multiple clients are asking to
    // read the same object. But that should be rare.
    std::lock_guard<std::mutex> lock(item.mtx);

    Json::Value json;
    json["success"] = true;
    json["data"] = "Found";
    // TODO: use item to return more info.

    return callback(HttpResponse::newHttpJsonResponse(json));
}

////////////////////////////////////////////////////////////////////////////////

enum InputError {
    Ok,
    InvalidType,
};

static InputError addUIntData(Kernel* kernel, const Json::Value array) {
    std::vector<uint32_t> data;
    data.reserve(array.size());
    for (int i=0; i<array.size(); i++) {
        const auto arrayItem = array[i];
        if (!arrayItem.isUInt()) {
            return InvalidType;
            //return callback(makeFailedResponse("Outputs is not a list of integers"));
        }
        const auto value = arrayItem.asUInt();
        data.push_back(value);
    }

    kernel->addInputData<uint32_t>(data);
    return Ok;
}

static InputError addFloatData(Kernel* kernel, const Json::Value array) {
    std::vector<float> data;
    data.reserve(array.size());
    for (int i=0; i<array.size(); i++) {
        const auto arrayItem = array[i];
        if (!arrayItem.isDouble()) {
            return InvalidType;
            //return callback(makeFailedResponse("Outputs is not a list of integers"));
        }
        const auto value = arrayItem.asFloat();
        data.push_back(value);
    }

    kernel->addInputData<float>(data);
    return Ok;
}

void Server::updateKernel(const HttpRequestPtr& req, HttpCallback callback, const std::string& id) {
    auto jsonPtr = req->jsonObject();
    if (jsonPtr == nullptr) {
        return callback(makeFailedResponse("Invalid JSON"));
    }

    const Json::Value &json = *jsonPtr;
    if (!json["update"].isString()) {
        return callback(makeFailedResponse("Empty or missing action"));
    }

    const auto updateType = json["update"].asString();

    if (updateType == "input") {
        // TODO: separate method.
    } else {
        return callback(makeFailedResponse("Unrecognized update action"));
    }

    // input() action.
    if (!json["index"].isUInt()) {
        return callback(makeFailedResponse("Missing index"));
    } else if (!json["data"].isArray()) {
        return callback(makeFailedResponse("Missing input data"));
    }

    const auto data = json["data"];

    auto itemPtr = [this, &id]() -> std::shared_ptr<KernelItem> {
        // It is possible that the item is being removed while another
        // thread tries to look it up. The mutex here prevents that from
        // happening.
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_kernels.find(id);
        if (it == m_kernels.end()) {
            return nullptr;
        }
        return it->second;
    }();

    if (itemPtr == nullptr) {
        return callback(makeFailedResponse("Kernel not found"));
    }

    auto& item = *itemPtr;
    std::lock_guard<std::mutex> lock(item.mtx);

    int ret;

    switch (item.type) {
        case DataType::FLOAT:
            ret = addFloatData(&item.kernel, data);
            if (ret != Ok) {
                return callback(makeFailedResponse("Input data is not a list of floats"));
            }
            break;
        case DataType::UINT32:
            ret = addUIntData(&item.kernel, data);
            if (ret != Ok) {
                return callback(makeFailedResponse("Input data is not a list of integers"));
            }
            break;
        default:
            return callback(makeFailedResponse("Invalid data type (internal)"));
    }
    // addUIntData(Kernel* kernel, const Json::Value data);

    //return callback(makeFailedResponse("Outputs is not a list of integers"));

    // std::vector<uint32_t> data{bytes.begin(), bytes.end()};
    //kernel->addInputData<uint32_t>(data);

    Json::Value res;
    res["success"] = true;
    res["data"] = "Data updated successfully (I think)";
    callback(HttpResponse::newHttpJsonResponse(std::move(res)));
}

////////////////////////////////////////////////////////////////////////////////

void Server::executeKernel(const HttpRequestPtr& req, HttpCallback callback, const std::string& id) {
    auto itemPtr = [this, &id]() -> std::shared_ptr<KernelItem> {
        // It is possible that the item is being removed while another
        // thread tries to look it up. The mutex here prevents that from
        // happening.
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_kernels.find(id);
        if (it == m_kernels.end()) {
            return nullptr;
        }
        return it->second;
    }();

    if (itemPtr == nullptr) {
        return callback(makeFailedResponse("Kernel not found"));
    }

    auto& item = *itemPtr;
    // Prevents another thread from writing to the same item while this
    // thread reads. Could cause blockage if multiple clients are asking to
    // read the same object. But that should be rare.
    std::lock_guard<std::mutex> lock(item.mtx);

    Kernel* kernel = &item.kernel;

    kernel->execute();

    // get the output data.
    uint32_t *c = kernel->getOutputData<uint32_t>(0);
    // print out results in 'c'
    std::cout << "c: [" << c[0] << ", " << c[1] << ", " << c[2] << ", " << c[3]
              << "]" << std::endl;

    Json::Value json;
    json["success"] = true;
    json["data"] = "Let's see if it worked (fingers crossed)";

    return callback(HttpResponse::newHttpJsonResponse(json));
}
