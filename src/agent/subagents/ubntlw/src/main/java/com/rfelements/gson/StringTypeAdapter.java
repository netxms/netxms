package com.rfelements.gson;
import com.google.gson.JsonDeserializationContext;
import com.google.gson.JsonDeserializer;
import com.google.gson.JsonElement;
import com.google.gson.JsonParseException;

import java.lang.reflect.Type;
/**
 * @author Pichanič Ján
 */
public class StringTypeAdapter implements JsonDeserializer<String> {

	@Override
	public String deserialize(JsonElement json, Type typeOfT, JsonDeserializationContext context) throws JsonParseException {
		if (json.toString().contains("{}"))
			return null;
		String converted = json.getAsString();
		if(converted.contains("\\/8"))
			converted = converted.substring(1, converted.length()-3);
		return converted;
	}
}
