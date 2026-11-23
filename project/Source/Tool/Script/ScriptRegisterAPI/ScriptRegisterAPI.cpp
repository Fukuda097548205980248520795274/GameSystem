#include "ScriptRegisterAPI.hpp"
#include "Engine.hpp"
#include "Matrix/Matrix4x4/Matrix4x4.hpp"
#include "Matrix/Matrix3x3/Matrix3x3.hpp"

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



    // Matrix3x3 の登録
    scriptEngine->RegisterObjectType("Matrix3x3", sizeof(Matrix3x3), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Matrix3x3>());

    // 配列アクセス用メソッドの登録
    scriptEngine->RegisterObjectMethod("Matrix3x3", "float Get(uint row, uint col) const",
        asFUNCTIONPR([](const Matrix3x3& m, uint32_t row, uint32_t col) -> float {
            if (row < 3 && col < 3) return m.m[row][col];
            return 0.0f;
            }, (const Matrix3x3&, uint32_t, uint32_t), float), asCALL_CDECL_OBJFIRST);

    scriptEngine->RegisterObjectMethod("Matrix3x3", "void Set(uint row, uint col, float value)",
        asFUNCTIONPR([](Matrix3x3& m, uint32_t row, uint32_t col, float value) {
            if (row < 3 && col < 3) m.m[row][col] = value;
            }, (Matrix3x3&, uint32_t, uint32_t, float), void), asCALL_CDECL_OBJFIRST);



    // Matrix4x4 の登録
    scriptEngine->RegisterObjectType("Matrix4x4", sizeof(Matrix4x4), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<Matrix4x4>());

    // デフォルトコンストラクタ
    scriptEngine->RegisterObjectBehaviour("Matrix4x4", asBEHAVE_CONSTRUCT, "void f()",
        asFUNCTIONPR([](void* memory) { new(memory) Matrix4x4(); }, (void*), void), asCALL_CDECL_OBJLAST);

    // 配列アクセス用メソッド
    scriptEngine->RegisterObjectMethod("Matrix4x4", "float Get(uint row, uint col) const",
        asFUNCTIONPR([](const Matrix4x4& m, uint32_t row, uint32_t col) -> float {
            if (row < 4 && col < 4) return m.m[row][col];
            return 0.0f;
            }, (const Matrix4x4&, uint32_t, uint32_t), float), asCALL_CDECL_OBJFIRST);

    scriptEngine->RegisterObjectMethod("Matrix4x4", "void Set(uint row, uint col, float value)",
        asFUNCTIONPR([](Matrix4x4& m, uint32_t row, uint32_t col, float value) {
            if (row < 4 && col < 4) m.m[row][col] = value;
            }, (Matrix4x4&, uint32_t, uint32_t, float), void), asCALL_CDECL_OBJFIRST);

    // メソッドの登録
    scriptEngine->RegisterObjectMethod("Matrix4x4", "Matrix4x4 Inverse() const", asMETHOD(Matrix4x4, Inverse), asCALL_THISCALL);
    scriptEngine->RegisterObjectMethod("Matrix4x4", "Matrix4x4 Transpose() const", asMETHOD(Matrix4x4, Transpose), asCALL_THISCALL);

    // 演算子の登録 (自己代入系)
    scriptEngine->RegisterObjectMethod("Matrix4x4", "Matrix4x4& opAddAssign(const Matrix4x4 &in)", asMETHODPR(Matrix4x4, operator+=, (const Matrix4x4&), Matrix4x4&), asCALL_THISCALL);
    scriptEngine->RegisterObjectMethod("Matrix4x4", "Matrix4x4& opSubAssign(const Matrix4x4 &in)", asMETHODPR(Matrix4x4, operator-=, (const Matrix4x4&), Matrix4x4&), asCALL_THISCALL);
    scriptEngine->RegisterObjectMethod("Matrix4x4", "Matrix4x4& opMulAssign(const Matrix4x4 &in)", asMETHODPR(Matrix4x4, operator*=, (const Matrix4x4&), Matrix4x4&), asCALL_THISCALL);
    scriptEngine->RegisterObjectMethod("Matrix4x4", "Matrix4x4& opMulAssign(float)", asMETHODPR(Matrix4x4, operator*=, (float), Matrix4x4&), asCALL_THISCALL);

    // 演算子の登録 (二項演算子)
    scriptEngine->RegisterObjectMethod("Matrix4x4", "Matrix4x4 opAdd(const Matrix4x4 &in) const", asFUNCTIONPR(operator+, (const Matrix4x4&, const Matrix4x4&), Matrix4x4), asCALL_CDECL_OBJFIRST);
    scriptEngine->RegisterObjectMethod("Matrix4x4", "Matrix4x4 opSub(const Matrix4x4 &in) const", asFUNCTIONPR(operator-, (const Matrix4x4&, const Matrix4x4&), Matrix4x4), asCALL_CDECL_OBJFIRST);
    scriptEngine->RegisterObjectMethod("Matrix4x4", "Matrix4x4 opMul(const Matrix4x4 &in) const", asFUNCTIONPR(operator*, (const Matrix4x4&, const Matrix4x4&), Matrix4x4), asCALL_CDECL_OBJFIRST);
    scriptEngine->RegisterObjectMethod("Matrix4x4", "Matrix4x4 opMul(float) const", asFUNCTIONPR(operator*, (const Matrix4x4&, float), Matrix4x4), asCALL_CDECL_OBJFIRST);
    scriptEngine->RegisterObjectMethod("Matrix4x4", "Matrix4x4 opMul_r(float) const", asFUNCTIONPR(operator*, (float, const Matrix4x4&), Matrix4x4), asCALL_CDECL_OBJLAST);


    // Matrix4x4 生成/変換用 グローバル関数の登録
    scriptEngine->RegisterGlobalFunction("Matrix4x4 MakeIdentityMatrix4x4()", asFUNCTION(MakeIdentityMatrix4x4), asCALL_CDECL);
    scriptEngine->RegisterGlobalFunction("Matrix4x4 Make2DScaleMatrix4x4(const Vector2 &in)", asFUNCTION(Make2DScaleMatrix4x4), asCALL_CDECL);
    scriptEngine->RegisterGlobalFunction("Matrix4x4 Make3DScaleMatrix4x4(const Vector3 &in)", asFUNCTION(Make3DScaleMatrix4x4), asCALL_CDECL);
    scriptEngine->RegisterGlobalFunction("Matrix4x4 Make3DRotateXMatrix4x4(float)", asFUNCTION(Make3DRotateXMatrix4x4), asCALL_CDECL);
    scriptEngine->RegisterGlobalFunction("Matrix4x4 Make3DRotateYMatrix4x4(float)", asFUNCTION(Make3DRotateYMatrix4x4), asCALL_CDECL);
    scriptEngine->RegisterGlobalFunction("Matrix4x4 Make3DRotateZMatrix4x4(float)", asFUNCTION(Make3DRotateZMatrix4x4), asCALL_CDECL);
    scriptEngine->RegisterGlobalFunction("Matrix4x4 Make3DRotateMatrix4x4(const Vector3 &in)", asFUNCTIONPR(Make3DRotateMatrix4x4, (const Vector3&), Matrix4x4), asCALL_CDECL);
    scriptEngine->RegisterGlobalFunction("Matrix4x4 Make2DTranslateMatrix4x4(const Vector2 &in)", asFUNCTION(Make2DTranslateMatrix4x4), asCALL_CDECL);
    scriptEngine->RegisterGlobalFunction("Matrix4x4 Make3DTranslateMatrix4x4(const Vector3 &in)", asFUNCTION(Make3DTranslateMatrix4x4), asCALL_CDECL);
    scriptEngine->RegisterGlobalFunction("Matrix4x4 Make3DAffineMatrix4x4(const Vector2 &in scale, const Vector3 &in rot, const Vector2 &in trans)", asFUNCTIONPR(Make3DAffineMatrix4x4, (const Vector2&, const Vector3&, const Vector2&), Matrix4x4), asCALL_CDECL);
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
