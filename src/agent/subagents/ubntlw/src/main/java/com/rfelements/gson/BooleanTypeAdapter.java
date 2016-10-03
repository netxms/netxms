package com.rfelements.gson;

import com.google.gson.JsonDeserializationContext;
import com.google.gson.JsonDeserializer;
import com.google.gson.JsonElement;
import com.google.gson.JsonParseException;

import java.lang.reflect.Type;

/**
 * @author Pichanič Ján
 */
public class BooleanTypeAdapter implements JsonDeserializer<Boolean> {

    @Override
    public Boolean deserialize(JsonElement json, Type typeOfT, JsonDeserializationContext context) throws JsonParseException {
        if (json.toString().length() <= 1) {
            int code = json.getAsInt();
            return code == 0 ? false : code == 1 ? true : null;
        }
        return json.getAsBoolean();
    }
}
