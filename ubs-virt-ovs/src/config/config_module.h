//
 	 // Created by l00668584 on 2026/1/9.
 	 //

 	 #ifndef CONFIG_MODULE_H
 	 #define CONFIG_MODULE_H
 	 #include <cstdint>
 	 #include <string>

 	 namespace virt::ovs::config {
 	 struct configItem {
 	     std::string section{};
 	     std::string key{};
 	     std::string value{};
 	 };

 	 class ConfModule {
 	 public:
 	     static ConfModule &GetInstance()
 	     {
 	         static ConfModule instance;
 	         return instance;
 	     }
 	     uint32_t Init();
 	     /**
 	     * @brief 读取配置
 	     * @param [in] section: 配置节
 	     * @param [in] configKey: 配置参数key
 	     * @param [out] configVal: 配置参数值
 	     * @return UbseResult, 成功返回0, 失败返回非0
 	     */
 	     template <typename T>
 	     uint32_t GetConf(const std::string &section, const std::string &configKey, T &configVal);

 	 private:
 	     uint32_t GetUIntConf(const std::string &section, const std::string &configKey, uint32_t &configVal);

 	     uint32_t GetULongConf(const std::string &section, const std::string &configKey, uint64_t &configVal);

 	     uint32_t GetFloatConf(const std::string &section, const std::string &configKey, float &configVal);

 	     uint32_t GetStringConf(const std::string &section, const std::string &configKey, std::string &configVal);

 	     uint32_t GetBoolConf(const std::string &section, const std::string &configKey, bool &configVal);

 	     std::string configDefaultDir;
 	     std::string confCliDir;
 	 };

 	 template <typename T>
 	 uint32_t ConfModule::GetConf(const std::string &section, const std::string &configKey, T &configVal)
 	 {
 	     if constexpr (std::is_same_v<T, uint32_t>) {
 	         return GetUIntConf(section, configKey, configVal);
 	     }

 	     if constexpr (std::is_same_v<T, uint64_t>) {
 	         return GetULongConf(section, configKey, configVal);
 	     }

 	     if constexpr (std::is_same_v<T, float>) {
 	         return GetFloatConf(section, configKey, configVal);
 	     }

 	     if constexpr (std::is_same_v<T, std::string>) {
 	         return GetStringConf(section, configKey, configVal);
 	     }

 	     if constexpr (std::is_same_v<T, bool>) {
 	         return GetBoolConf(section, configKey, configVal);
 	     }
 	     return 1;
 	 }
 	 std::tuple<std::string, std::string, std::string> TrimConf(const std::string &section, const std::string &configKey,
 	                                                            const std::string &configVal = "");
 	 bool IsValidNumber(const std::string &str, bool allowFloating = false);

 	 } // namespace virt::ovs::config

 	 #endif // CONFIG_MODULE_H