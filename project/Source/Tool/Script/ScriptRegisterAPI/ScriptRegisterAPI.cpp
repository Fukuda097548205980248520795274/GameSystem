#include "ScriptRegisterAPI.hpp"
#include "Engine.hpp"

namespace
{
    /// @brief コンテキストからエンティティIDを取得するヘルパー
    /// @return 
    Entity GetActiveEntity()
    {
        asIScriptContext* ctx = asGetActiveContext();
        if (!ctx) return kNullEntity;
        return static_cast<Entity>(reinterpret_cast<uintptr_t>(ctx->GetUserData()));
    }

    /// @brief コンポーネントを取得するテンプレート関数
    /// @return 
    template<typename T>
    T GetComponentFromScript()
    {
        Entity entity = GetActiveEntity();
        auto registry = Engine::GetInstance()->GetRegistryECS();

        // エンティティが有効で、かつコンポーネントを持っている場合に取得
        if (entity != kNullEntity && registry->HasComponent<T>(entity))
        {
            return *registry->GetComponent<T>(entity);
        }

        // 未所持の場合はデフォルト値を返す
        return T();
    }

    /// @brief コンポーネントを適用するテンプレート関数
    /// @tparam T 
    /// @param component 
    template<typename T>
    void SetComponentFromScript(const T& component)
    {
        Entity entity = GetActiveEntity();
        auto registry = Engine::GetInstance()->GetRegistryECS();

        // エンティティが有効で、かつコンポーネントを持っている場合に適用
        if (entity != kNullEntity && registry->HasComponent<T>(entity))
        {
            *registry->GetComponent<T>(entity) = component;
        }
    }

    /// @brief コンポーネントのGet/Set関数をまとめて登録するヘルパー
    /// @tparam T 
    /// @param engine 
    /// @param typeName 
    template<typename T>
    void RegisterComponentAccessors(asIScriptEngine* scriptEngine, const std::string& typeName)
    {
        // 取得関数
        std::string getDecl = typeName + " Get" + typeName + "()";
        int r = scriptEngine->RegisterGlobalFunction(getDecl.c_str(), asFUNCTION(GetComponentFromScript<T>), asCALL_CDECL);

        // 設定関数
        std::string setDecl = "void Set" + typeName + "(const " + typeName + " &in)";
        r = scriptEngine->RegisterGlobalFunction(setDecl.c_str(), asFUNCTION(SetComponentFromScript<T>), asCALL_CDECL);
    }
}

/// @brief 数学APIをスクリプトエンジンに登録する
/// @param scriptEngine 
void Detail::ScriptRegisterAPI::RegisterMathAPI(asIScriptEngine* scriptEngine)
{
    // Vector3 の登録
    scriptEngine->RegisterObjectType("Vector3", sizeof(Vector3), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Vector3>());
    scriptEngine->RegisterObjectProperty("Vector3", "float x", asOFFSET(Vector3, x));
    scriptEngine->RegisterObjectProperty("Vector3", "float y", asOFFSET(Vector3, y));
    scriptEngine->RegisterObjectProperty("Vector3", "float z", asOFFSET(Vector3, z));

    // Vector2 の登録
    scriptEngine->RegisterObjectType("Vector2", sizeof(Vector2), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Vector2>());
    scriptEngine->RegisterObjectProperty("Vector2", "float x", asOFFSET(Vector2, x));
    scriptEngine->RegisterObjectProperty("Vector2", "float y", asOFFSET(Vector2, y));
}

/// @brief コンポーネントAPIをスクリプトエンジンに登録する
/// @param scriptEngine 
void Detail::ScriptRegisterAPI::RegisterComponentAPI(asIScriptEngine* scriptEngine)
{
    // Entity の登録
    scriptEngine->RegisterObjectType("Entity", sizeof(Entity), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Entity>());

    // TransformComponent の登録
    scriptEngine->RegisterObjectType("TransformComponent", sizeof(TransformComponent), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<TransformComponent>());
    scriptEngine->RegisterObjectProperty("TransformComponent", "Vector3 position", asOFFSET(TransformComponent, position));
    scriptEngine->RegisterObjectProperty("TransformComponent", "Vector3 rotation", asOFFSET(TransformComponent, rotation));
    scriptEngine->RegisterObjectProperty("TransformComponent", "Vector3 scale", asOFFSET(TransformComponent, scale));
    RegisterComponentAccessors<TransformComponent>(scriptEngine, "TransformComponent");
}

/// @brief エンジンAPIをスクリプトエンジンに登録する
/// @param scriptEngine 
void Detail::ScriptRegisterAPI::RegisterEngineAPI(asIScriptEngine* scriptEngine)
{
    // Log 関数の登録
    scriptEngine->RegisterGlobalFunction("void Log(const string &in)",
        asFUNCTIONPR([](const std::string& str)
            {
                Engine::GetInstance()->Log(LogLevel::Info, str);
            },
            (const std::string&), void), asCALL_CDECL);

    // DeltaTime 関数の登録
    scriptEngine->RegisterGlobalFunction("float GetDeltaTime()",
        asFUNCTIONPR([]() -> float
            {
                return Engine::GetInstance()->GetDeltaTime();
            },
            (), float), asCALL_CDECL);
}

/// @brief 入力ボタンAPIをスクリプトエンジンに登録する
/// @param scriptEngine 
void Detail::ScriptRegisterAPI::RegisterInputButtonAPI(asIScriptEngine* scriptEngine)
{
    // キーボード定数
    scriptEngine->RegisterEnum("Key");
    scriptEngine->RegisterEnumValue("Key", "W", DIK_W);
    scriptEngine->RegisterEnumValue("Key", "A", DIK_A);
    scriptEngine->RegisterEnumValue("Key", "S", DIK_S);
    scriptEngine->RegisterEnumValue("Key", "D", DIK_D);
    scriptEngine->RegisterEnumValue("Key", "Space", DIK_SPACE);
    scriptEngine->RegisterEnumValue("Key", "Enter", DIK_RETURN);
    scriptEngine->RegisterEnumValue("Key", "Escape", DIK_ESCAPE);
    scriptEngine->RegisterEnumValue("Key", "LShift", DIK_LSHIFT);
    scriptEngine->RegisterEnumValue("Key", "Up", DIK_UP);
    scriptEngine->RegisterEnumValue("Key", "Down", DIK_DOWN);
    scriptEngine->RegisterEnumValue("Key", "Left", DIK_LEFT);
    scriptEngine->RegisterEnumValue("Key", "Right", DIK_RIGHT);

    // マウスボタン定数
    scriptEngine->RegisterEnum("MouseButton");
    scriptEngine->RegisterEnumValue("MouseButton", "Left", 0);
    scriptEngine->RegisterEnumValue("MouseButton", "Right", 1);
    scriptEngine->RegisterEnumValue("MouseButton", "Middle", 2);

    // ゲームパッドボタン定数
    scriptEngine->RegisterEnum("GamepadButton");
    scriptEngine->RegisterEnumValue("GamepadButton", "DPadUp", XINPUT_GAMEPAD_DPAD_UP);
    scriptEngine->RegisterEnumValue("GamepadButton", "DPadDown", XINPUT_GAMEPAD_DPAD_DOWN);
    scriptEngine->RegisterEnumValue("GamepadButton", "DPadLeft", XINPUT_GAMEPAD_DPAD_LEFT);
    scriptEngine->RegisterEnumValue("GamepadButton", "DPadRight", XINPUT_GAMEPAD_DPAD_RIGHT);
    scriptEngine->RegisterEnumValue("GamepadButton", "Start", XINPUT_GAMEPAD_START);
    scriptEngine->RegisterEnumValue("GamepadButton", "Back", XINPUT_GAMEPAD_BACK);
    scriptEngine->RegisterEnumValue("GamepadButton", "LeftThumb", XINPUT_GAMEPAD_LEFT_THUMB);
    scriptEngine->RegisterEnumValue("GamepadButton", "RightThumb", XINPUT_GAMEPAD_RIGHT_THUMB);
    scriptEngine->RegisterEnumValue("GamepadButton", "LeftShoulder", XINPUT_GAMEPAD_LEFT_SHOULDER);
    scriptEngine->RegisterEnumValue("GamepadButton", "RightShoulder", XINPUT_GAMEPAD_RIGHT_SHOULDER);
    scriptEngine->RegisterEnumValue("GamepadButton", "A", XINPUT_GAMEPAD_A);
    scriptEngine->RegisterEnumValue("GamepadButton", "B", XINPUT_GAMEPAD_B);
    scriptEngine->RegisterEnumValue("GamepadButton", "X", XINPUT_GAMEPAD_X);
    scriptEngine->RegisterEnumValue("GamepadButton", "Y", XINPUT_GAMEPAD_Y);
}

/// @brief 入力処理APIをスクリプトエンジンに登録する
/// @param scriptEngine 
void Detail::ScriptRegisterAPI::RegisterInputAPI(asIScriptEngine* scriptEngine)
{
    // Input 名前空間を設定
    scriptEngine->SetDefaultNamespace("Input");

    // キーボード
    scriptEngine->RegisterGlobalFunction("bool GetKeyPress(uint8 key)",
        asFUNCTIONPR([](uint8_t key) -> bool {
            return Engine::GetInstance()->GetInput()->GetKeyPress(key);
            }, (uint8_t), bool), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("bool GetKeyTrigger(uint8 key)",
        asFUNCTIONPR([](uint8_t key) -> bool {
            return Engine::GetInstance()->GetInput()->GetKeyTrigger(key);
            }, (uint8_t), bool), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("bool GetKeyRelease(uint8 key)",
        asFUNCTIONPR([](uint8_t key) -> bool {
            return Engine::GetInstance()->GetInput()->GetKeyRelease(key);
            }, (uint8_t), bool), asCALL_CDECL);

    // マウス
    scriptEngine->RegisterGlobalFunction("bool GetMousePress(uint button)",
        asFUNCTIONPR([](uint32_t button) -> bool {
            return Engine::GetInstance()->GetInput()->GetMousePress(button);
            }, (uint32_t), bool), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("bool GetMouseTrigger(uint button)",
        asFUNCTIONPR([](uint32_t button) -> bool {
            return Engine::GetInstance()->GetInput()->GetMouseTrigger(button);
            }, (uint32_t), bool), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("bool GetMouseRelease(uint button)",
        asFUNCTIONPR([](uint32_t button) -> bool {
            return Engine::GetInstance()->GetInput()->GetMouseRelease(button);
            }, (uint32_t), bool), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("Vector2 GetMouseVelocity()",
        asFUNCTIONPR([]() -> Vector2 {
            return Engine::GetInstance()->GetInput()->GetMouseVelocity();
            }, (), Vector2), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("bool GetMouseWheelUp()",
        asFUNCTIONPR([]() -> bool {
            return Engine::GetInstance()->GetInput()->GetMouseWheelUp();
            }, (), bool), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("bool GetMouseWheelDown()",
        asFUNCTIONPR([]() -> bool {
            return Engine::GetInstance()->GetInput()->GetMouseWheelDown();
            }, (), bool), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("float GetMouseWheelVelocity()",
        asFUNCTIONPR([]() -> float {
            return Engine::GetInstance()->GetInput()->GetMouseWheelVelocity();
            }, (), float), asCALL_CDECL);

    // ゲームパッド
    scriptEngine->RegisterGlobalFunction("bool IsGamepadEnable(uint padIndex = 0)",
        asFUNCTIONPR([](uint32_t padIndex) -> bool {
            return Engine::GetInstance()->GetInput()->IsGamepadEnable(padIndex);
            }, (uint32_t), bool), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("bool GetGamepadButtonPress(uint button, uint padIndex = 0)",
        asFUNCTIONPR([](uint32_t button, uint32_t padIndex) -> bool {
            return Engine::GetInstance()->GetInput()->GetGamepadButtonPress(padIndex, button);
            }, (uint32_t, uint32_t), bool), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("bool GetGamepadButtonTrigger(uint button, uint padIndex = 0)",
        asFUNCTIONPR([](uint32_t button, uint32_t padIndex) -> bool {
            return Engine::GetInstance()->GetInput()->GetGamepadButtonTrigger(padIndex, button);
            }, (uint32_t, uint32_t), bool), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("bool GetGamepadButtonRelease(uint button, uint padIndex = 0)",
        asFUNCTIONPR([](uint32_t button, uint32_t padIndex) -> bool {
            return Engine::GetInstance()->GetInput()->GetGamepadButtonRelease(padIndex, button);
            }, (uint32_t, uint32_t), bool), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("Vector2 GetGamepadLeftStick(uint padIndex = 0)",
        asFUNCTIONPR([](uint32_t padIndex) -> Vector2 {
            return Engine::GetInstance()->GetInput()->GetGamepadLeftStick(padIndex);
            }, (uint32_t), Vector2), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("Vector2 GetGamepadRightStick(uint padIndex = 0)",
        asFUNCTIONPR([](uint32_t padIndex) -> Vector2 {
            return Engine::GetInstance()->GetInput()->GetGamepadRightStick(padIndex);
            }, (uint32_t), Vector2), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("float GetGamepadLeftTrigger(uint padIndex = 0)",
        asFUNCTIONPR([](uint32_t padIndex) -> float {
            return Engine::GetInstance()->GetInput()->GetGamepadLeftTrigger(padIndex);
            }, (uint32_t), float), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("float GetGamepadRightTrigger(uint padIndex = 0)",
        asFUNCTIONPR([](uint32_t padIndex) -> float {
            return Engine::GetInstance()->GetInput()->GetGamepadRightTrigger(padIndex);
            }, (uint32_t), float), asCALL_CDECL);

    scriptEngine->RegisterGlobalFunction("void GamepadVibration(float leftPower, float rightPower, uint padIndex = 0)",
        asFUNCTIONPR([](float left, float right, uint32_t padIndex) {
            Engine::GetInstance()->GetInput()->GamepadVibration(padIndex, left, right);
            }, (float, float, uint32_t), void), asCALL_CDECL);

    // デフォルトの名前空間に戻す
    scriptEngine->SetDefaultNamespace("");
}
