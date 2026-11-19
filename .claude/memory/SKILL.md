19:08:53 [INFO] Opening Tindog and logging in...
19:09:04 [INFO] Login complete.
19:09:23 [INFO] Starting swipe loop (target: 20 likes)...
19:09:28 [WARNING] No like button found in time, retrying...
19:09:35 [WARNING] No like button found in time, retrying...
19:09:42 [WARNING] No like button found in time, retrying...
19:09:49 [WARNING] No like button found in time, retrying...
19:09:56 [WARNING] No like button found in time, retrying...
19:10:04 [WARNING] No like button found in time, retrying...
19:10:11 [WARNING] No like button found in time, retrying...
19:10:18 [WARNING] No like button found in time, retrying...
19:10:25 [WARNING] No like button found in time, retrying...
19:10:32 [WARNING] No like button found in time, retrying...
19:10:36 [ERROR] Unexpected browser error.
Traceback (most recent call last):
  File "C:\Users\lenovo\OneDrive\Desktop\Python course\DAY_50.py", line 229, in main
    report = swipe_loop(driver, logger, args.max_swipes, args.timeout)
  File "C:\Users\lenovo\OneDrive\Desktop\Python course\DAY_50.py", line 157, in swipe_loop
    like_button = wait.until(
        EC.element_to_be_clickable((By.CSS_SELECTOR, "button.btn-like"))
    )
  File "D:\Apps\python\Lib\site-packages\selenium\webdriver\support\wait.py", line 112, in until
    value = method(self._driver)
  File "D:\Apps\python\Lib\site-packages\selenium\webdriver\support\expected_conditions.py", line 518, in _predicate
    target = driver.find_element(*target)  # grab element at locator
  File "D:\Apps\python\Lib\site-packages\selenium\webdriver\remote\webdriver.py", line 899, in find_element
    return self.execute(Command.FIND_ELEMENT, {"using": by, "value": value})["value"]
           ~~~~~~~~~~~~^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "D:\Apps\python\Lib\site-packages\selenium\webdriver\remote\webdriver.py", line 492, in execute
    self.error_handler.check_response(response)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^^^^^^^^^
  File "D:\Apps\python\Lib\site-packages\selenium\webdriver\remote\errorhandler.py", line 232, in check_response
    raise exception_class(message, screen, stacktrace)
selenium.common.exceptions.InvalidSessionIdException: Message: invalid session id: session deleted as the browser has closed the connection
from disconnected: not connected to DevTools
  (Session info: chrome=151.0.7922.76); For documentation on this error, please visit: https://www.selenium.dev/documentation/webdriver/troubleshooting/errors#invalidsessionidexception
Stacktrace:
	chromedriver!GetHandleVerifier [0x7ff778348935+14ce5]
	chromedriver!GetHandleVerifier [0x7ff778348990+14d40]
	chromedriver!(No symbol) [0x7ff777e75afd]
	chromedriver!(No symbol) [0x7ff777e61372]
	chromedriver!(No symbol) [0x7ff777e87236]
	chromedriver!(No symbol) [0x7ff777f00bc0]
	chromedriver!(No symbol) [0x7ff777f1dd92]
	chromedriver!(No symbol) [0x7ff777ec308c]
	chromedriver!(No symbol) [0x7ff777ec3fb3]
	chromedriver!GetHandleVerifier [0x7ff77897e5fb+64a9ab]
	chromedriver!GetHandleVerifier [0x7ff778978b02+644eb2]
	chromedriver!GetHandleVerifier [0x7ff77899e39e+66a74e]
	chromedriver!GetHandleVerifier [0x7ff778365d6e+3211e]
	chromedriver!GetHandleVerifier [0x7ff77836e51c+3a8cc]
	chromedriver!GetHandleVerifier [0x7ff778352844+1ebf4]
	chromedriver!GetHandleVerifier [0x7ff7783529d4+1ed84]
	chromedriver!GetHandleVerifier [0x7ff778335d07+20b7]
	KERNEL32!BaseThreadInitThunk [0x7ffab99be957+17]
	ntdll!RtlUserThreadStart [0x7ffaba40ad6c+2c]

19:10:37 [ERROR] Could not capture screenshot.
19:10:37 [INFO] Report saved to C:\Users\lenovo\OneDrive\Desktop\Python course\reports\report_20260811_191037.json
19:10:39 [INFO] Browser closed. Done.
19:52:55 [INFO] Opening Tindog and logging in...
19:52:59 [ERROR] Unexpected browser error.
Traceback (most recent call last):
  File "C:\Users\lenovo\OneDrive\Desktop\Python course\DAY_50.py", line 232, in main
    login(driver, logger, email, password)
    ~~~~~^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "C:\Users\lenovo\OneDrive\Desktop\Python course\DAY_50.py", line 112, in login
    login_button = driver.find_element(By.XPATH, value='//*[text()="Log in"]')
  File "D:\Apps\python\Lib\site-packages\selenium\webdriver\remote\webdriver.py", line 899, in find_element
    return self.execute(Command.FIND_ELEMENT, {"using": by, "value": value})["value"]
           ~~~~~~~~~~~~^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "D:\Apps\python\Lib\site-packages\selenium\webdriver\remote\webdriver.py", line 492, in execute
    self.error_handler.check_response(response)
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^^^^^^^^^
  File "D:\Apps\python\Lib\site-packages\selenium\webdriver\remote\errorhandler.py", line 232, in check_response
    raise exception_class(message, screen, stacktrace)
selenium.common.exceptions.NoSuchWindowException: Message: no such window: target window already closed
from unknown error: web view not found
  (Session info: chrome=151.0.7922.76)
Stacktrace:
	chromedriver!GetHandleVerifier [0x7ff778348935+14ce5]
	chromedriver!GetHandleVerifier [0x7ff778348990+14d40]
	chromedriver!(No symbol) [0x7ff777e75afd]
	chromedriver!(No symbol) [0x7ff777e4c9f2]
	chromedriver!(No symbol) [0x7ff777f00966]
	chromedriver!(No symbol) [0x7ff777f1dd92]
	chromedriver!(No symbol) [0x7ff777ec308c]
	chromedriver!(No symbol) [0x7ff777ec3fb3]
	chromedriver!GetHandleVerifier [0x7ff77897e5fb+64a9ab]
	chromedriver!GetHandleVerifier [0x7ff778978b02+644eb2]
	chromedriver!GetHandleVerifier [0x7ff77899e39e+66a74e]
	chromedriver!GetHandleVerifier [0x7ff778365d6e+3211e]
	chromedriver!GetHandleVerifier [0x7ff77836e51c+3a8cc]
	chromedriver!GetHandleVerifier [0x7ff778352844+1ebf4]
	chromedriver!GetHandleVerifier [0x7ff7783529d4+1ed84]
	chromedriver!GetHandleVerifier [0x7ff778335d07+20b7]
	KERNEL32!BaseThreadInitThunk [0x7ffab99be957+17]
	ntdll!RtlUserThreadStart [0x7ffaba40ad6c+2c]

19:52:59 [ERROR] Could not capture screenshot.
19:52:59 [INFO] Report saved to C:\Users\lenovo\OneDrive\Desktop\Python course\reports\report_20260811_195259.json
19:53:01 [INFO] Browser closed. Done.
19:53:48 [INFO] Opening Tindog and logging in...
19:53:59 [INFO] Login complete.
19:54:02 [INFO] Location access granted.
19:54:16 [INFO] Starting swipe loop (target: 20 likes)...
19:56:59 [INFO] Liked profile #1
20:00:56 [INFO] Liked profile #2
20:01:16 [INFO] Liked profile #3
20:03:59 [INFO] Liked profile #4
20:06:43 [INFO] Liked profile #5
20:10:21 [INFO] Liked profile #6
20:11:56 [INFO] Liked profile #7
20:16:14 [INFO] Liked profile #8
20:17:35 [INFO] Liked profile #9
20:21:10 [INFO] Liked profile #10
20:24:00 [INFO] Liked profile #11
20:28:01 [INFO] Liked profile #12
20:31:22 [INFO] Liked profile #13
20:33:41 [INFO] Liked profile #14
20:37:01 [INFO] Liked profile #15
20:39:14 [INFO] Liked profile #16
20:43:26 [INFO] Liked profile #17
20:43:32 [WARNING] No like button found in time, retrying...
20:43:39 [WARNING] No like button found in time, retrying...
20:43:46 [WARNING] No like button found in time, retrying...
20:43:53 [WARNING] No like button found in time, retrying...
20:44:01 [WARNING] No like button found in time, retrying...
20:44:08 [WARNING] No like button found in time, retrying...
20:44:15 [WARNING] No like button found in time, retrying...
20:44:22 [WARNING] No like button found in time, retrying...
20:44:29 [WARNING] No like button found in time, retrying...
20:44:36 [WARNING] No like button found in time, retrying...
20:44:44 [WARNING] No like button found in time, retrying...
20:44:51 [WARNING] No like button found in time, retrying...
20:44:58 [WARNING] No like button found in time, retrying...
20:45:05 [WARNING] No like button found in time, retrying...
20:45:12 [WARNING] No like button found in time, retrying...
20:45:20 [WARNING] No like button found in time, retrying...
20:45:27 [WARNING] No like button found in time, retrying...
20:45:34 [WARNING] No like button found in time, retrying...
20:45:41 [WARNING] No like button found in time, retrying...
20:45:48 [WARNING] No like button found in time, retrying...
20:45:55 [WARNING] No like button found in time, retrying...
20:46:03 [WARNING] No like button found in time, retrying...
20:48:06 [INFO] Liked profile #18
20:56:45 [INFO] Report saved to C:\Users\lenovo\OneDrive\Desktop\Python course\reports\report_20260811_205645.json
20:56:48 [INFO] Browser closed. Done.
