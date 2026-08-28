# Web Services

This skill provides access to NetXMS web service definitions and the ability to call them against monitored objects. Use it to:

- **Inspect definitions**: List and examine configured web services - URL, HTTP method, authentication type, headers, timeouts, and caching
- **Query a service**: Retrieve the live response document from a REST or HTTP API and analyze its structure
- **Verify extraction paths**: Confirm that a JSON or XML path resolves to the expected value before it is used for data collection
- **Create and update definitions**: Configure new web services or adjust existing ones
- **Troubleshoot**: Diagnose web service metrics that return no data, wrong values, or errors

## Concepts

### Definition, path, and metric name

A **web service definition** describes how to reach an API: URL, HTTP method, request body, authentication, headers, and TLS options. It is global - one definition is reused across many monitored objects.

A **data extraction path** selects one value (or a list of values) inside the response document. Path syntax depends on the response format:
- JSON responses use JSONPath, written as a slash-separated path such as `/data/cpu/load` or `/items/0/value`
- XML responses use XPath, such as `//sensor[@id='1']/value`

A data collection item with origin `webService` combines the two in its metric name:

```
<definition name>:<path>
```

For example, definition `PrometheusNode` and path `/data/result/0/value/1` gives metric name `PrometheusNode:/data/result/0/value/1`. If the definition's URL or headers contain macros, arguments are passed in parentheses after the name: `WeatherAPI(Berlin):/current/temp_c`.

### Macros and arguments

URL, request body, and header values are expanded in the context of the queried object, so they may contain object macros (for example `%{ipAddr}` or `%{objectName}`) and positional arguments `$1`, `$2`, and so on. Positional arguments come from the `args` parameter of `query-web-service` and `test-web-service-path`, and from the parentheses in the DCI metric name. Because expansion depends on the object, always query the same object the data collection item will be created on.

### Web service proxy and response cache

Requests are not made by the server - they are made by the NetXMS agent on the queried object's effective web service proxy. If that agent is unreachable, the query fails even though the API itself is healthy.

The agent caches response documents per URL for the definition's cache retention time, so several metrics based on one definition cause only one HTTP request. This affects the two query functions differently:
- `query-web-service` always bypasses the cache and hits the live service
- `test-web-service-path` may be served from the cache, exactly as data collection would be

Calling `query-web-service` first refreshes the cached document, so the usual order below also gives the path test fresh data.

## Setting up a new web service metric

1. Call `list-web-services` to check whether a suitable definition already exists.
2. If not, create one with `create-web-service`. Ask the user for the URL and authentication details.
3. Call `query-web-service` for the object that will collect the data, and read the returned document to understand its structure.
4. Propose one or more extraction paths and verify each with `test-web-service-path`. Never propose a path without verifying it - a path that looks correct against the document may still fail, for example because of the array indexing rules of the parser.
5. Use the `dciMetricName` returned by a successful path test as the `metric` argument of `create-metric` in the data-collection skill, with `origin` set to `webService`.

## Troubleshooting a failing web service metric

1. `get-web-service` for the definition used by the metric - check URL, method, and authentication.
2. `query-web-service` for the affected object. Interpret the result:
   - Connection or agent errors mean the web service proxy agent is unreachable, not that the API is broken
   - An HTTP response code of 401 or 403 points at authentication; 404 at a wrong URL or unexpanded macro
   - Check the returned `url` field - it shows the URL after macro expansion, which is where wrong or missing arguments become visible
3. If the document is returned correctly, the problem is in the path: run `test-web-service-path` with the metric's path. A "path did not match anything" result means the document structure differs from what the path expects - re-read the document and propose a corrected path.

## Security

- Passwords are never returned by any function in this skill, and bearer tokens are reported as `***`. Header values that look like secrets are also hidden. Do not ask the user to repeat a credential just to display it.
- Everything passed to `create-web-service` and `update-web-service` is sent to the configured AI provider. When credentials must be set, prefer telling the user to enter them in the management client rather than typing them into the chat.
- Reading and changing definitions requires the "Configure web service definitions" system right; querying a web service requires the "Query web services" right on the target object. Report an access denied result to the user rather than trying another object.

## Best Practices

- Query the object the data will actually be collected on, not an arbitrary node - macro expansion and the effective proxy both depend on it.
- Large response documents are truncated at 64KB. If the `truncated` flag is set, do not assume the part you cannot see is absent; narrow the request instead, for example by adding an API-side filter to the URL.
- Set `cache_retention_time` on definitions whose API is expensive or rate limited, so many metrics share one request.
- Use `type: list` in `test-web-service-path` when the path is intended for instance discovery rather than a single metric value.
- When creating a definition for an API served over HTTPS with a valid certificate, enable `verify_certificate` and `verify_host`.
